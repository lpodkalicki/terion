/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

namespace {

Theme makeTheme(const int palette, const int density)
{
    Theme theme = palette == 1 ? Theme::light() :
        palette == 2 ? Theme::ocean() : Theme::dark();
    theme.density = density == 0 ? ComponentDensity::Compact :
        density == 2 ? ComponentDensity::Spacious : ComponentDensity::Comfortable;
    theme.focus_color = theme.palette.warning;
    return theme;
}

std::string densityName(const int density)
{
    return density == 0 ? "Compact" : density == 2 ? "Spacious" : "Comfortable";
}

} /* End anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> density{1};
    State<int> palette{0};
    State<std::string> username{"Ada Lovelace"};
    State<std::string> role{"admin"};
    State<bool> alerts{true};

    const auto updateTheme = [&] { app.setTheme(makeTheme(palette.get(),density.get())); };
    updateTheme();
    app.render([&] {
        const auto screen = app.backend().size();
        const int width = std::max(1,screen.width);
        const int height = std::max(1,screen.height);
        return Paper({.children={
            Typography({.text="TERION | #138 Layout, Theme & Component Consistency",
                .variant=TypographyVariant::H2}),
            Typography({.text="Resize the terminal, click controls, or use Tab/Shift+Tab. Esc to quit.",
                .color=ColorVariant::Info}),
            Grid({.children={
                Button({.text="Density: "+densityName(density.get()),
                    .key="density",.on_click=[&] {
                        density.set((density.get()+1)%3); updateTheme();
                    }}),
                Button({.text="Palette: "+std::string(palette.get()==0?"Dark":
                    palette.get()==1?"Light":"Ocean"),.key="palette",
                    .on_click=[&] { palette.set((palette.get()+1)%3); updateTheme(); }}),
                Button({.text="Reset",.key="reset",.variant=ButtonVariant::Contained,
                    .on_click=[&] {density.set(1); palette.set(0); updateTheme();}})
            },.layout={.width=std::max(0,width-4),.gap_token=SpacingToken::Small},
                .columns=3,.min_column_width=22}),
            Divider({}),
            Grid({.children={
                Card({.title="Actions",.children={
                    Button({.text="Primary",.key="action-primary",.variant=ButtonVariant::Contained}),
                    Button({.text="Secondary",.key="action-secondary",.color=ColorVariant::Info}),
                    Button({.text="Disabled",.disabled=true}),
                    Typography({.text="All share the same density metrics."})
                },.layout={.padding_token=SpacingToken::Small,.gap_token=SpacingToken::Small}}),
                Card({.title="Form fields",.children={
                    Typography({.text="Name"}),
                    TextField({.value=username.get(),.placeholder="Type a name",.key="name",
                        .on_change=[username](std::string value) mutable {
                            username.set(std::move(value));
                        }}),
                    Select({.options={{"Admin","admin"},{"Operator","operator"},{"Viewer","viewer"}},
                        .value=role.get(),.key="role",
                        .on_change=[role](std::string value) mutable {role.set(std::move(value));}}),
                    Checkbox({.label="Show alerts",.key="alerts",.checked=alerts.get(),
                        .on_change=[alerts](bool next) mutable {alerts.set(next);}})
                },.layout={.padding_token=SpacingToken::Small,.gap_token=SpacingToken::Small}}),
                Card({.title="Responsive content",.children={
                    Typography({.text="Grid: 3 / 2 / 1 columns as available width changes."}),
                    Typography({.text="Theme-driven spacing tokens"}),
                    Typography({.text="Focus outline uses theme.focus_color"}),
                    Typography({.text="No manual sizing of child controls"})
                },.layout={.padding_token=SpacingToken::Small,.gap_token=SpacingToken::Medium}})
            },.layout={.width=std::max(0,width-4),.gap_token=SpacingToken::Small},
                .columns=3,.min_column_width=30}),
            Divider({}),
            Typography({.text="Screen: "+std::to_string(width)+"x"+std::to_string(height)+
                " | Density: "+densityName(density.get())+
                " | Selected role: "+role.get()+" | Focus: Tab / Shift+Tab"})
        },.layout={.width=width,.height=height,.padding_token=SpacingToken::Small,
            .gap_token=SpacingToken::Small}});
    });
    return app.run();
}
