/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

enum class CounterAction { Increment, Decrement, Reset };

int counterReducer(int count,CounterAction action)
{
    switch (action)
    {
        case CounterAction::Increment: return count + 1;
        case CounterAction::Decrement: return count - 1;
        case CounterAction::Reset: return 0;
    }
    return count;
}

const auto Counter = component([] {
    auto [count,dispatch] = useReducer(counterReducer,0);
    return Column({.children = {
        Typography({.text = "Count: " + std::to_string(count)}),
        Row({.children = {
            Button({.text = "+",.onClick = [dispatch] { dispatch(CounterAction::Increment); }}),
            Button({.text = "-",.onClick = [dispatch] { dispatch(CounterAction::Decrement); }}),
            Button({.text = "Reset",.onClick = [dispatch] { dispatch(CounterAction::Reset); }})
        },.layout = {.gap = 1}})
    },.layout = {.gap = 1}});
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([] { return Column({.children = {Counter()},.layout = {.padding = 2}}); });
    return app.run();
}
