/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_CLIPBOARD_H
#define TERION_CLIPBOARD_H

#include <functional>
#include <string>
#include <string_view>

namespace terion {

// Process-local clipboard shared by editable widgets. OS clipboard integration
// is intentionally separate: terminals do not universally support reading it.
class Clipboard
{
public:
    // The system writer is scoped to Application event dispatch, never retained
    // across applications. Reading the OS clipboard is not portable over OSC 52.
    using Writer = std::function<void(std::string_view)>;
    class ScopedWriter
    {
    public:
        explicit ScopedWriter(Writer writer) : previous_(std::move(sink())) { sink() = std::move(writer); }
        ~ScopedWriter() { sink() = std::move(previous_); }
        ScopedWriter(const ScopedWriter&) = delete;
        ScopedWriter& operator=(const ScopedWriter&) = delete;
    private:
        Writer previous_;
    };

    static void set(std::string_view text)
    {
        storage().assign(text);
        if (sink()) sink()(storage());
    }
    [[nodiscard]] static const std::string& get() { return storage(); }
    static void clear() { storage().clear(); }

private:
    static Writer& sink()
    {
        static thread_local Writer writer;
        return writer;
    }
    static std::string& storage()
    {
        static std::string value;
        return value;
    }
}; /* End of class Clipboard */

} /* End of namespace terion */

#endif /* TERION_CLIPBOARD_H */
