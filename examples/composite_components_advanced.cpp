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
    State<std::size_t> step{1};
    State<bool> validated{false};
    State<bool> finished{false};
    State<bool> vertical{false};
    State<bool> keep_mounted{true};
    State<bool> multiple{true};
    State<std::vector<std::string>> expanded{{"profile"}};
    State<std::string> notes{"Editable note survives accordion collapse"};
    State<std::string> current{"/workspace/projects/terion/examples/composite"};
    State<std::string> message{"Ready — try choosing step 3 before checking validation."};

    const std::vector<Breadcrumbs::Item> route{
        {"Home","/"}, {"Workspace","/workspace"},
        {"Projects","/workspace/projects"}, {"Terion","/workspace/projects/terion"},
        {"Examples","/workspace/projects/terion/examples"},
        {"Composite Components","/workspace/projects/terion/examples/composite"}
    };

    app.render([&] {
        const auto active=step.get();
        const auto valid=validated.get();
        const auto done=finished.get();
        const auto is_vertical=vertical.get();
        const auto keep=keep_mounted.get();
        const auto multi=multiple.get();
        const auto opened=expanded.get();
        const auto note=notes.get();
        const auto location=current.get();
        const auto status=message.get();

        std::vector<Accordion::Item> items{
            {.id="profile",.title="Project details",.children={
                Typography({.text="Edit the note, collapse this section and open it again:"}),
                TextField({.value=note,.key="project-note",
                    .on_change=[notes](std::string value) mutable {notes.set(std::move(value));}})
            },.description="Editable text"},
            {.id="settings",.title="Rendering options",.children={
                Typography({.text="Hidden panels remain reconciled when keep_mounted is enabled."}),
                Typography({.text="Hidden widgets are not focusable."})
            },.description="Keep mounted"},
            {.id="tips",.title="Keyboard help",.children={
                Typography({.text="Tab / Shift+Tab moves focus; Enter selects focused buttons."}),
                Typography({.text="Use the ellipsis menu in Breadcrumbs to navigate hidden ancestors."})
            },.description="Navigation"}
        };
        const auto all_ids=Accordion::expandAll(items);
        auto visible_route=route;
        for (std::size_t i=0; i<route.size(); ++i)
        {
            if (route[i].value != location) continue;
            visible_route.resize(i+1);
            break;
        }

        return Paper({.children={
            Typography({.text="TERION | #136b Composite Components 2.0",.variant=TypographyVariant::H2}),
            Typography({.text="Stepper — independent statuses, validation, vertical layout"}),
            Stepper({.steps={
                    {.label="Project"},
                    {.label="Configure",.status=(!valid && active==1)?StepStatus::Error:StepStatus::Auto,
                        .description="Configuration must be validated"},
                    {.label="Review",.description="Inspect the configuration"},
                    {.label="Finish",.description="Complete the wizard"}},
                .active_step=active,.linear=false,
                .on_change=[step,message](std::size_t index) mutable {
                    step.set(index);
                    message.set("Selected step "+std::to_string(index+1));
                },.completed=done,
                .orientation=is_vertical?StepperOrientation::Vertical:StepperOrientation::Horizontal,
                .on_before_change=[valid,message](std::size_t from,std::size_t to) mutable {
                    const bool allowed=to<=from || valid;
                    if (!allowed) message.set("Navigation blocked: validate configuration first.");
                    return allowed;
                }}),
            Row({.children={
                Button({.text="Back",.disabled=active==0,
                    .on_click=[step,active]() mutable {if (active>0) step.set(active-1);}}),
                Button({.text="Next",.disabled=(active>=3 || !valid),
                    .on_click=[step,active]() mutable {if (active<3) step.set(active+1);}}),
                Checkbox({.label="Valid",.checked=valid,
                    .on_change=[validated](bool value) mutable {validated.set(value);}}),
                Checkbox({.label="Finished",.checked=done,
                    .on_change=[finished](bool value) mutable {finished.set(value);}}),
                Checkbox({.label="Vertical",.checked=is_vertical,
                    .on_change=[vertical](bool value) mutable {vertical.set(value);}})
            },.layout={.gap=1}}),
            Typography({.text="Accordion — expand/collapse, optional state retention"}),
            Row({.children={
                Button({.text="Expand all",.disabled=!multi,
                    .on_click=[expanded,all_ids]() mutable {expanded.set(all_ids);}}),
                Button({.text="Collapse all",.on_click=[expanded]() mutable {expanded.set(Accordion::collapseAll());}}),
                Checkbox({.label="Multiple",.checked=multi,
                    .on_change=[multiple,expanded](bool value) mutable {
                        multiple.set(value);
                        if (!value) expanded.set({});
                    }}),
                Checkbox({.label="Keep mounted",.checked=keep,
                    .on_change=[keep_mounted](bool value) mutable {keep_mounted.set(value);}})
            },.layout={.gap=1}}),
            Accordion({.items=std::move(items),.expanded=opened,.multiple=multi,
                .on_change=[expanded](std::vector<std::string> next) mutable {
                    expanded.set(std::move(next));
                },.keep_mounted=keep}),
            Typography({.text="Breadcrumbs — collapsible path and grapheme-safe truncation"}),
            Breadcrumbs({.items=std::move(visible_route),.on_navigate=[current,message](std::string path) mutable {
                    current.set(path);
                    message.set("Navigated to "+path);
                },.max_visible_items=4,.max_label_width=14}),
            Row({.children={
                Button({.text="Restore full path",.on_click=[current,route]() mutable {
                    current.set(route.back().value);
                }}),
                Typography({.text="Selected: "+location})
            },.layout={.gap=1}}),
            Typography({.text="Status: "+status}),
            Typography({.text="Tab: focus   |   Enter: activate   |   Mouse: click   |   Esc: quit"})
        },.layout={.padding=1,.gap=1}});
    });
    return app.run();
}
