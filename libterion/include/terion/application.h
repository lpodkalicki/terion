/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_APPLICATION_H
#define TERION_APPLICATION_H

#include <functional>
#include <memory>
#include <terion/terminal.h>
#include <terion/render/painter.h>
#include <terion/render/screen_buffer.h>
#include <terion/render/differential_renderer.h>

namespace terion {

class Application
{
public:
    using PaintCallback = std::function<void(Painter&, Size)>;

    explicit Application(std::unique_ptr<TerminalBackend> backend);

    void render(PaintCallback callback)
    {
        paint_ = std::move(callback);
    }

    void renderOnce();
    int run();

    void quit() noexcept
    {
        running_ = false;
    }

    [[nodiscard]] TerminalBackend& backend() const
    {
        return *backend_;
    }

private:
    std::unique_ptr<TerminalBackend> backend_;
    DifferentialRenderer renderer_;
    PaintCallback paint_;
    bool running_ = false;
}; /* End of class Application */

} /* End of namespace terion */

#endif /* TERION_APPLICATION_H */
