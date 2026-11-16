/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::size_t> step{0};
    State<std::vector<std::string>> expanded{{"details"}};
    State<std::string> location{"project"};

    app.render([&] {
        const auto active=step.get();
        const auto open=expanded.get();
        const auto path=location.get();
        // Build an owned path with a stable lifetime before passing it to Props.
        // Chained conditionals containing temporary vectors trigger GCC's
        // -Wdangling-pointer diagnostic with -O3 on some Linux toolchains.
        std::vector<Breadcrumbs::Item> breadcrumb_items{
            {"Home", "home"},
            {"Workspace", "workspace"},
            {"Project", "project"}
        };
        if (path == "home") breadcrumb_items.resize(1);
        else if (path == "workspace") breadcrumb_items.resize(2);

        return Paper({.children={
            Typography({.text="Terion | #134 Composite Components",.variant=TypographyVariant::H2}),
            Typography({.text="STEP 1 | Stepper (select a completed step or use Next)"}),
            Stepper({.steps={{"Project"},{"Configure"},{"Review"},{"Finish",true}},
                .active_step=active,.linear=true,
                .on_change=[step](std::size_t index) mutable {step.set(index);}}),
            Row({.children={
                Button({.text="Back",.disabled=active==0,
                    .on_click=[step,active]() mutable {if (active>0) step.set(active-1);}}),
                Button({.text="Next",.disabled=active>=3,
                    .on_click=[step,active]() mutable {if (active<3) step.set(active+1);}})
            },.layout={.gap=1}}),
            Typography({.text="STEP 2 | Accordion (expand to inspect content)"}),
            Accordion({.items={
                {.id="details",.title="Project details",.children={
                    Typography({.text="A reusable composite widget built from Button, Column and Box."}),
                    Typography({.text="Click the header to expand or collapse."})}},
                {.id="settings",.title="Settings",.children={
                    Typography({.text="Multiple sections can remain open."}),
                    Checkbox({.label="Sample checkbox",.checked=true})}},
                {.id="help",.title="Help",.children={
                    Typography({.text="Mouse and keyboard focus work via regular Buttons."})}}
            },.expanded=open,.multiple=true,
                .on_change=[expanded](std::vector<std::string> next) mutable {expanded.set(std::move(next));}}),
            Typography({.text="STEP 3 | Breadcrumbs (navigate to an ancestor)"}),
            Breadcrumbs({.items=std::move(breadcrumb_items),
                .on_navigate=[location](std::string value) mutable {location.set(std::move(value));}}),
            Row({.children={
                Button({.text="Open project",.on_click=[location]() mutable {location.set("project");}}),
                Typography({.text="Current location: "+path})
            },.layout={.gap=1}})
        },.layout={.padding=1,.gap=1}});
    });
    return app.run();
}
