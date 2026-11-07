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
    Reconciler reconciler;
    reconciler.update(Component::column({
        Component::text("Alpha","a"),
        Component::text("Beta","b")
    }));
    const auto* root = reconciler.root();
    assert(root && root->children().size() == 2);
    const auto root_id = root->id();
    const auto a_id = root->children()[0]->id();
    const auto b_id = root->children()[1]->id();
    reconciler.update(Component::column({
        Component::text("Beta updated","b"),
        Component::text("Alpha","a")
    }));
    assert(reconciler.root()->id() == root_id);
    assert(reconciler.root()->children()[0]->id() == b_id);
    assert(reconciler.root()->children()[0]->text() == "Beta updated");
    assert(reconciler.root()->children()[1]->id() == a_id);
    reconciler.update(Component::column({Component::column({},"b")}));
    assert(reconciler.root()->children()[0]->id() != b_id);
    bool duplicate_rejected = false;
    try
    {
        reconciler.update(Component::column({Component::text("A","same"),Component::text("B","same")}));
    }
    catch (const std::invalid_argument&)
    {
        duplicate_rejected = true;
    }
    assert(duplicate_rejected);

    FocusManager focus;
    int first_calls = 0;
    int second_calls = 0;
    focus.add("first",[&](const Event&) { ++first_calls; return true; });
    focus.add("second",[&](const Event&) { ++second_calls; return true; });
    assert(focus.current() == "first");
    assert(focus.dispatch(Event{.key = Key::Character,.character = U'x'}));
    assert(first_calls == 1 && second_calls == 0);
    assert(focus.dispatch(Event{.key = Key::Tab}));
    assert(focus.current() == "second");
    assert(focus.dispatch(Event{.key = Key::Enter}));
    assert(second_calls == 1);
    assert(focus.previous() && focus.current() == "first");
    assert(focus.remove("first") && focus.current() == "second");
    assert(!focus.focus("missing"));

    InputDecoder decoder;
    decoder.feed("\033");
    assert(!decoder.nextEvent());
    assert(decoder.nextEvent(true)->key == Key::Escape);
    decoder.feed("\033[");
    assert(!decoder.nextEvent());
    decoder.feed("A");
    assert(decoder.nextEvent()->key == Key::Up);
    decoder.feed("\033[3~\033OD");
    assert(decoder.nextEvent()->key == Key::Delete);
    assert(decoder.nextEvent()->key == Key::Left);
    decoder.feed("\xC5");
    assert(!decoder.nextEvent());
    decoder.feed("\x81");
    const auto letter = decoder.nextEvent();
    assert(letter && letter->key == Key::Character && letter->character == U'Ł');
    decoder.feed("\t\r");
    assert(decoder.nextEvent()->key == Key::Tab);
    assert(decoder.nextEvent()->key == Key::Enter);
    decoder.feed("\xE2");
    assert(!decoder.nextEvent());
    assert(decoder.nextEvent(true)->character == 0xFFFD);
    decoder.feed("\033[1;5C");
    assert(decoder.nextEvent()->key == Key::Right);
    decoder.feed("\033[99~");
    assert(decoder.nextEvent()->key == Key::Unknown);
    assert(!decoder.nextEvent());

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
    terminal.resize({.width = 14,.height = 4});
    const auto resized = terminal.pollEvent();
    assert(resized && resized->key == Key::Resize);
    assert(resized->size == (Size{.width = 14,.height = 4}));
    terminal.resize({.width = 12,.height = 3});
    assert(terminal.pollEvent()->key == Key::Resize);
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
    observed->clearOutput();
    std::string label = "First";
    app.render([&] {
        return Component::column({Component::text(label,"label")});
    });
    app.renderOnce();
    assert(observed->output().find("F") != std::string::npos);
    const auto label_id = app.reconciler().root()->children()[0]->id();
    ScreenBuffer component_buffer({.width = 20,.height = 5});
    Painter component_painter(component_buffer);
    app.reconciler().root()->paint(component_painter);
    assert(component_buffer.at(0,0).grapheme == "F");
    label = "Next";
    observed->clearOutput();
    app.renderOnce();
    assert(app.reconciler().root()->children()[0]->id() == label_id);
    assert(observed->output().find("N") != std::string::npos);
    bool received = false;
    app.onEvent([&](const Event& event) { received = event.key == Key::Enter; return received; });
    assert(app.dispatchEvent(Event{.key = Key::Enter}));
    assert(received);
    received = false;
    app.focus().add("editor",[&](const Event& event) { return event.key == Key::Escape; });
    assert(app.dispatchEvent(Event{.key = Key::Escape}));
    assert(!received);
    assert(app.focus().remove("editor"));
    assert(app.dispatchEvent(Event{.key = Key::Escape}));
    // The headless backend allows run() to finish deterministically without a TTY.
    observed->push(Event{.key = Key::Escape});
    assert(app.run() == 0);
    std::cout << "Terion tests passed\n";
}
