#include <terion/terion.h>
#include <memory>
using namespace terion;
int main() {
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([](Painter& painter, Size) {
        painter.text(2,1,"Hello Terion!",TextStyle{.foreground = Color::rgb(100,210,255), .bold = true});
        painter.text(2,3,"Press ESC to exit.");
        painter.text(2,5,"Unicode: Łódź / 界 / 🚀");
    });
    return app.run();
}
