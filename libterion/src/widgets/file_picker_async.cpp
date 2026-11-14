/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <exception>
#include <terion/components/file_picker_async.h>

namespace terion {

FilePickerAsyncBrowser::FilePickerAsyncBrowser() : FilePickerAsyncBrowser(Options{}) {}

FilePickerAsyncBrowser::FilePickerAsyncBrowser(Options options) : options_(options)
{
    worker_ = std::thread([this] { run(); });
}

FilePickerAsyncBrowser::~FilePickerAsyncBrowser()
{
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
        queued_.reset();
    }
    wake_.notify_one();
    if (worker_.joinable()) worker_.join();
}

bool FilePickerAsyncBrowser::Key::operator==(const Key& other) const
{
    return directory == other.directory && options.extensions == other.options.extensions &&
           options.search == other.options.search && options.show_hidden == other.options.show_hidden;
}

bool FilePickerAsyncBrowser::isPendingLocked(const Key& key) const
{
    return (queued_ && queued_->generation == generation_ && queued_->key == key) ||
           (active_ && active_->generation == generation_ && active_->key == key);
}

void FilePickerAsyncBrowser::scheduleLocked(const Key& key)
{
    if (isPendingLocked(key)) return;
    // Last-request-wins: fast typing should not enqueue a long series of scans.
    queued_ = Work{.key = key,.generation = generation_};
    wake_.notify_one();
}

FilePickerAsyncBrowser::Snapshot FilePickerAsyncBrowser::request(
    const std::filesystem::path& directory,const FilePickerBrowser::Options& options)
{
    const Key key{.directory = directory.lexically_normal(),.options = options};
    std::lock_guard lock(mutex_);
    Snapshot result;
    const auto now = std::chrono::steady_clock::now();
    const auto iter = std::find_if(cached_.begin(),cached_.end(),
        [&](const CacheEntry& entry) { return entry.key == key; });
    if (iter != cached_.end())
    {
        result.listing = iter->listing;
        result.ready = true;
        iter->last_used = ++access_counter_;
        if (options_.cache_ttl.count() <= 0 || now-iter->refreshed < options_.cache_ttl)
            return result;
    }
    scheduleLocked(key);
    result.loading = true;
    return result;
}

bool FilePickerAsyncBrowser::consumeUpdates() noexcept
{
    return updates_.exchange(false,std::memory_order_acq_rel);
}

void FilePickerAsyncBrowser::invalidate(const std::filesystem::path& directory)
{
    const auto normalized = directory.lexically_normal();
    std::lock_guard lock(mutex_);
    ++generation_;
    std::erase_if(cached_,[&](const CacheEntry& entry) { return entry.key.directory == normalized; });
    // Keep active work running, but invalidate its generation so it cannot
    // insert a stale result into the cache after refresh.
    if (queued_ && queued_->key.directory == normalized) queued_.reset();
    updates_.store(true,std::memory_order_release);
}

void FilePickerAsyncBrowser::invalidateAll()
{
    std::lock_guard lock(mutex_);
    ++generation_;
    cached_.clear();
    queued_.reset();
    updates_.store(true,std::memory_order_release);
}

std::size_t FilePickerAsyncBrowser::cachedCount() const
{
    std::lock_guard lock(mutex_);
    return cached_.size();
}

void FilePickerAsyncBrowser::run()
{
    for (;;)
    {
        Work work;
        {
            std::unique_lock lock(mutex_);
            wake_.wait(lock,[this] { return stop_ || queued_.has_value(); });
            if (stop_) return;
            work = std::move(*queued_);
            queued_.reset();
            active_ = work;
        }
        FilePickerListing listing;
        try
        {
            listing = FilePickerBrowser::list(work.key.directory,work.key.options);
        }
        catch (const std::exception& error)
        {
            listing.directory = work.key.directory;
            listing.error = error.what();
        }
        {
            std::lock_guard lock(mutex_);
            active_.reset();
            if (!stop_ && work.generation == generation_)
            {
                auto iter = std::find_if(cached_.begin(),cached_.end(),
                    [&](const CacheEntry& entry) { return entry.key == work.key; });
                if (iter != cached_.end()) cached_.erase(iter);
                if (cached_.size() >= std::max<std::size_t>(1,options_.cache_capacity))
                {
                    const auto least_recent = std::min_element(cached_.begin(),cached_.end(),
                        [](const CacheEntry& lhs,const CacheEntry& rhs) {
                            return lhs.last_used < rhs.last_used;
                        });
                    cached_.erase(least_recent);
                }
                cached_.push_back({.key = std::move(work.key),
                    .listing = std::make_shared<FilePickerListing>(std::move(listing)),
                    .refreshed = std::chrono::steady_clock::now(),
                    .generation = work.generation,.last_used = ++access_counter_});
                updates_.store(true,std::memory_order_release);
            }
            else if (!stop_)
            {
                // Wake the UI so it can reissue an invalidated in-flight query.
                updates_.store(true,std::memory_order_release);
            }
        }
    }
}

} /* End of namespace terion */
