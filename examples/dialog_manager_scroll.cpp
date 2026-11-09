/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

namespace {

const auto OpenDocument = component([] {
    auto dialogs = useDialogManager();
    return Button({.text = "Read document",.onClick = [dialogs] {
        std::vector<Component> paragraphs;
        for (int i = 1; i <= 60; ++i)
            paragraphs.push_back(Typography({.text = "Paragraph " + std::to_string(i) +
                ": This is a line of the document shown inside the dialog."}));
        dialogs->open({.title = "Long document",.width = 72,
            .children = {Column({.children = std::move(paragraphs),.layout = {.gap = 1}})}});
    }});
});

} /* End of anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    auto dialogs = std::make_shared<DialogManager>();
    app.render([&] {
        return DialogProvider(dialogs,Column({.children = {
            Typography({.text = "DialogManager / scrolling demo"}),
            OpenDocument()
        },.layout = {.padding = 2,.gap = 1}}));
    });
    return app.run();
}
