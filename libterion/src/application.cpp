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

int Application::run()
{
    running_ = true;
    Size lastSize {.width = -1,.height = -1};
    while (running_)
    {
        const auto size = backend_->size();
        if (size != lastSize) { renderer_.invalidate(); lastSize = size; }
        renderOnce();
        if (const auto event = backend_->pollEvent(); event && event->key == Key::Escape) quit();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    return 0;
}

