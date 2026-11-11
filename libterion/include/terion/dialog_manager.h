/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_DIALOG_MANAGER_H
#define TERION_DIALOG_MANAGER_H

#include <memory>
#include <stdexcept>
#include <utility>
#include <terion/components/dialog.h>
#include <terion/context.h>
#include <terion/state.h>

namespace terion {

// One active modal at a time. The manager owns the dialog declaration, not a RenderNode.
class DialogManager : public std::enable_shared_from_this<DialogManager>
{
public:
    void open(Dialog::Props props)
    {
        props.open = true;
        current_.set(std::make_shared<Dialog::Props>(std::move(props)));
    }

    void close() { current_.set(nullptr); }

    [[nodiscard]] bool isOpen() const { return static_cast<bool>(current_.get()); }

    [[nodiscard]] Component render()
    {
        auto current = current_.get();
        if (!current) return Fragment({});
        auto props = *current;
        const auto original_close = props.on_close;
        const auto weak = weak_from_this();
        props.on_close = [weak,original_close] {
            if (auto manager = weak.lock()) manager->close();
            if (original_close) original_close();
        };
        props.key = "managed-dialog";
        return Dialog(std::move(props));
    }

private:
    State<std::shared_ptr<Dialog::Props>> current_{nullptr};
};

namespace detail {

inline const Context<std::shared_ptr<DialogManager>> dialog_manager_context(nullptr);

} /* End of namespace detail */

// Use from a mounted function component under DialogProvider.
[[nodiscard]] inline std::shared_ptr<DialogManager> useDialogManager()
{
    auto manager = useContext(detail::dialog_manager_context);
    if (!manager) throw std::logic_error("useDialogManager() requires DialogProvider");
    return manager;
}

[[nodiscard]] inline Component DialogProvider(std::shared_ptr<DialogManager> manager,
    Component child,std::string key = {})
{
    if (!manager) throw std::invalid_argument("DialogProvider requires a manager");
    return ContextProvider(detail::dialog_manager_context,manager,
        std::vector<Component>{std::move(child),manager->render()},std::move(key));
}

} /* End of namespace terion */

#endif /* TERION_DIALOG_MANAGER_H */
