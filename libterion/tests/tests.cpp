/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

using namespace terion;

int main()
{
    ScreenBuffer screen({.width = 12,.height = 3});
    Painter painter(screen);
    painter.text(0,0,"Łódź");
    assert(screen.at(0,0).grapheme == "Ł");
    assert(screen.at(3,0).grapheme == "ź");
    painter.text(0,1,"界");
    assert(screen.at(0,1).width == 2);
    assert(screen.at(1,1).width == 0);
    painter.put(1,1,"A");
    assert(screen.at(0,1).grapheme == " ");
    assert(screen.at(1,1).grapheme == "A");
    assert(isValidUtf8("Łódź"));
    assert(!isValidUtf8("\xFF"));
    HeadlessTerminalBackend terminal({12,3});
    DifferentialRenderer renderer;
    renderer.render(screen,terminal);
    assert(!terminal.output().empty());
    terminal.clearOutput();
    renderer.render(screen,terminal);
    assert(terminal.output().empty());
    painter.put(2,1,"B");
    renderer.render(screen,terminal);
    assert(terminal.output().find("B") != std::string::npos);
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    auto* observed = backend.get();
    Application app(std::move(backend));
    app.render([](Painter& p, Size) { p.text(0,0,"Terion"); });
    app.renderOnce();
    assert(observed->output().find("Terion") != std::string::npos);
    std::cout << "Terion tests passed\n";
}
