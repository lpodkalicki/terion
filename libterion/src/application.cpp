/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <stdexcept>
#include <thread>
#include <terion/application.h>

using namespace terion;

Application::Application(std::unique_ptr<TerminalBackend> backend) :
    backend_(std::move(backend))
{
    if (!backend_) throw std::invalid_argument("backend is null");
}

void Application::renderOnce()
{
    ScreenBuffer buffer(backend_->size());
    Painter painter(buffer);
    if (paint_) paint_(painter,buffer.size());
    renderer_.render(buffer,*backend_);
}

bool Application::dispatchEvent(const Event& event)
{
    if (event.key == Key::Resize) renderer_.invalidate();
    if (focus_.dispatch(event)) return true;
    if (on_event_ && on_event_(event)) return true;
    if (event.key == Key::Escape)
    {
        quit();
        return true;
    }
    return false;
}

int Application::run()
{
    running_ = true;
    Size lastSize {.width = -1,.height = -1};
    while (running_)
    {
        const auto size = backend_->size();
        if (size != lastSize) { renderer_.invalidate(); lastSize = size; }
        renderOnce();
        // Drain a bounded number of pending events to avoid delaying input under load.
        for (int i = 0; i < 64 && running_; ++i)
        {
            const auto event = backend_->pollEvent();
            if (!event) break;
            dispatchEvent(*event);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    return 0;
}

