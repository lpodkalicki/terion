/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FILE_PICKER_ASYNC_H
#define TERION_COMPONENTS_FILE_PICKER_ASYNC_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include <terion/components/file_picker_browser.h>

namespace terion {

// Filesystem enumeration always runs on a worker thread. No callback runs on that
// thread: call consumeUpdates() from an Application timer, then invalidate the UI.
// A new request supersedes any *queued* (not yet started) query. An in-flight
// filesystem call cannot be interrupted; destroying the browser waits for it.
class FilePickerAsyncBrowser
{
public:
    struct Options
    {
        std::size_t cache_capacity = 16;  // clamped to at least one entry
        std::chrono::milliseconds cache_ttl = std::chrono::seconds(10); // zero: never expires
    }; /* End of struct Options */

    struct Snapshot
    {
        std::shared_ptr<const FilePickerListing> listing;
        bool ready = false;
        bool loading = false;
    }; /* End of struct Snapshot */

    FilePickerAsyncBrowser();
    explicit FilePickerAsyncBrowser(Options options);
    ~FilePickerAsyncBrowser();

    FilePickerAsyncBrowser(const FilePickerAsyncBrowser&) = delete;
    FilePickerAsyncBrowser& operator=(const FilePickerAsyncBrowser&) = delete;

    // Non-blocking. Returns a cached snapshot if available, otherwise loading.
    [[nodiscard]] Snapshot request(const std::filesystem::path& directory,
                                   const FilePickerBrowser::Options& options = {});
    // Safe for the Application event-loop thread; no callbacks on the worker.
    [[nodiscard]] bool consumeUpdates() noexcept;
    void invalidate(const std::filesystem::path& directory);
    void invalidateAll();
    [[nodiscard]] std::size_t cachedCount() const;

private:
    struct Key
    {
        std::filesystem::path directory;
        FilePickerBrowser::Options options;
        [[nodiscard]] bool operator==(const Key& other) const;
    }; /* End of struct Key */

    struct Work
    {
        Key key;
        std::size_t generation = 0;
    }; /* End of struct Work */

    struct CacheEntry
    {
        Key key;
        std::shared_ptr<const FilePickerListing> listing;
        std::chrono::steady_clock::time_point refreshed;
        std::size_t generation = 0;
        std::size_t last_used = 0;
    }; /* End of struct CacheEntry */

    void run();
    void scheduleLocked(const Key& key);
    [[nodiscard]] bool isPendingLocked(const Key& key) const;

    Options options_;
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    bool stop_ = false;
    std::optional<Work> queued_;
    std::optional<Work> active_;
    std::vector<CacheEntry> cached_;
    std::size_t generation_ = 0;
    std::size_t access_counter_ = 0;
    std::atomic_bool updates_{false};
    // Must start after all synchronization state is constructed.
    std::thread worker_;
}; /* End of class FilePickerAsyncBrowser */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FILE_PICKER_ASYNC_H */
