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
    painter.text(3,1,"e\xCC\x81");
    assert(screen.at(3,1).grapheme == "e\xCC\x81");
    painter.text(11,1,"界");
    assert(screen.at(11,1).grapheme == " ");
    painter.text(11,1,"A\xCC\x81");
    assert(screen.at(11,1).grapheme == "A\xCC\x81");
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
    terminal.clearOutput();
    painter.text(5,1,"XY");
    renderer.render(screen,terminal);
    assert(terminal.output().find("XY") != std::string::npos);
    assert(terminal.output().find("\033[2J") == std::string::npos);
    assert(terminal.output().find("\033[2;7H") == std::string::npos);
    terminal.clearOutput();
    painter.text(7,1,"界");
    renderer.render(screen,terminal);
    terminal.clearOutput();
    painter.put(8,1,"Z");
    renderer.render(screen,terminal);
    assert(terminal.output().find(" Z") != std::string::npos);
    assert(screen.at(7,1).grapheme == " ");
    assert(screen.at(8,1).grapheme == "Z");
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    auto* observed = backend.get();
    Application app(std::move(backend));
    app.render([](Painter& p, Size) { p.text(0,0,"Terion"); });
    app.renderOnce();
    assert(observed->output().find("Terion") != std::string::npos);
    std::cout << "Terion tests passed\n";
}
