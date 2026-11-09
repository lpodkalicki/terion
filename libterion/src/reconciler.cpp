/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/reconciler.h>
#include <terion/hooks.h>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

using namespace terion;

namespace {

void validateKeys(const Component& component)
{
    if (component.props().type == Component::Type::TreeView)
    {
        std::unordered_set<std::string> tree_keys;
        const auto check = [&](const auto& self, const std::vector<TreeItem>& items) -> void {
            for (const auto& item : items)
            {
                if (item.key.empty() || !tree_keys.insert(item.key).second)
                    throw std::invalid_argument("TreeView items require unique, nonempty keys");
                self(self,item.children);
            }
        };
        check(check,component.props().tree_items);
    }
    std::unordered_set<std::string> keys;
    for (const auto& child : component.props().children)
    {
        if (!child.props().key.empty() && !keys.insert(child.props().key).second)
            throw std::invalid_argument("duplicate component key among siblings");
        validateKeys(child);
    }
}

} /* End of anonymous namespace */

void Reconciler::update(const Component& component)
{
    // Reject visible duplicate keys before running user-defined component functions.
    validateKeys(component);
    ++generation_;
    const auto retired_before = retired_hooks_.size();
    try
    {
        auto expanded = expand(component,"root");
        // A root fragment has no layout box; use the existing root column as its host.
        if (expanded.props().type == Component::Type::Fragment)
            expanded = Component::column(expanded.props().children);
        validateKeys(expanded);
        stats_ = {};
        reconcile(root_,expanded);
    }
    catch (...)
    {
        // A failed frame must not accidentally commit its scheduled effects.
        for (auto& [path,record] : hooks_)
        {
            (void)path;
            for (auto& slot : record.slots)
            {
                auto* effect = std::any_cast<std::shared_ptr<detail::EffectBase>>(&slot);
                if (effect && *effect) (*effect)->cancelPending();
            }
        }
        // Replacements of function identities are not committed on failed frames.
        for (std::size_t i = retired_hooks_.size(); i > retired_before; --i)
        {
            auto& [path,previous] = retired_hooks_[i-1];
            hooks_[path] = std::move(previous);
        }
        retired_hooks_.resize(retired_before);
        throw;
    }
    for (auto it = hooks_.begin(); it != hooks_.end();)
    {
        if (it->second.generation == generation_) { ++it; continue; }
        retired_hooks_.emplace_back(it->first,std::move(it->second));
        it = hooks_.erase(it);
    }
}

void Reconciler::commitEffects()
{
    // Unmounted instances clean up before any new effects start.
    retired_hooks_.clear();
    for (auto& [path,record] : hooks_)
    {
        (void)path;
        for (auto& slot : record.slots)
        {
            auto* effect = std::any_cast<std::shared_ptr<detail::EffectBase>>(&slot);
            if (effect && *effect) (*effect)->commit();
        }
    }
}

Component Reconciler::expand(const Component& component, const std::string& path)
{
    const auto& props = component.props();
    if (props.type == Component::Type::ThemeScope)
    {
        if (!props.theme_override || props.children.size() != 1)
            throw std::invalid_argument("invalid theme provider");
        const auto apply_theme = [&](const auto& self, const Component& node) -> Component {
            auto actual = node.props();
            if (actual.theme_override) return node; // A nested provider wins.
            actual.theme_override = props.theme_override;
            for (auto& child : actual.children) child = self(self,child);
            return Component(std::move(actual));
        };
        return apply_theme(apply_theme,expand(props.children.front(),path+"/theme"));
    }
    if (props.type == Component::Type::ErrorBoundary)
    {
        if (!props.error_fallback || props.children.size() != 1)
            throw std::invalid_argument("invalid error boundary");

        // Keep the failed branch isolated, including hook identity replacements.
        const auto child_path = path + "/error-boundary/child";
        const auto belongs_to_child = [&](const std::string& candidate) {
            return candidate == child_path ||
                (candidate.size() > child_path.size() &&
                candidate.compare(0,child_path.size(),child_path) == 0 &&
                candidate[child_path.size()] == '/');
        };
        std::unordered_map<std::string,HookRecord> saved_hooks;
        for (const auto& [name,record] : hooks_)
            if (belongs_to_child(name)) saved_hooks.emplace(name,record);
        const auto retired_before = retired_hooks_.size();

        Component result = Component::column({});
        try
        {
            result = expand(props.children.front(),child_path);
        }
        catch (const std::exception& error)
        {
            // Pending effects from an abandoned render must never be committed.
            for (auto it = hooks_.begin(); it != hooks_.end();)
            {
                if (!belongs_to_child(it->first)) { ++it; continue; }
                for (auto& slot : it->second.slots)
                {
                    auto* effect = std::any_cast<std::shared_ptr<detail::EffectBase>>(&slot);
                    if (effect && *effect) (*effect)->cancelPending();
                }
                it = hooks_.erase(it);
            }
            for (std::size_t i = retired_before; i < retired_hooks_.size(); ++i)
            {
                if (!belongs_to_child(retired_hooks_[i].first)) continue;
                for (auto& slot : retired_hooks_[i].second.slots)
                {
                    auto* effect = std::any_cast<std::shared_ptr<detail::EffectBase>>(&slot);
                    if (effect && *effect) (*effect)->cancelPending();
                }
            }
            retired_hooks_.resize(retired_before);
            for (auto& [name,record] : saved_hooks)
                hooks_.emplace(std::move(name),std::move(record));
            // The fallback is expanded outside the handler; its own errors propagate.
            result = expand(props.error_fallback(error.what()),path + "/error-boundary/fallback");
        }
        if (props.key.empty()) return result;
        auto actual = result.props();
        actual.key = props.key;
        return Component(std::move(actual));
    }
    if (props.type == Component::Type::ContextProvider)
    {
        if (!props.enter_context || !props.exit_context || props.children.size() != 1)
            throw std::invalid_argument("invalid context provider");
        props.enter_context();
        struct ContextExit
        {
            const std::function<void()>& callback;
            ~ContextExit() { callback(); }
        } exit{props.exit_context};
        auto child = expand(props.children.front(),path + "/provider" +
            std::to_string(props.key.size()) + ":" + props.key);
        if (props.key.empty()) return child;
        auto actual = child.props();
        actual.key = props.key;
        return Component(std::move(actual));
    }
    if (props.type == Component::Type::Function)
    {
        if (!props.render_function || !props.function_identity)
            throw std::invalid_argument("invalid function component");
        auto& record = hooks_[path];
        const bool same_identity = record.identity == props.function_identity;
        auto staged_slots = same_identity ? record.slots : std::vector<std::any>{};
        detail::HookContext context{staged_slots};
        Component result = [&] {
            detail::HookScope scope(context);
            return props.render_function();
        }();
        if (context.position != staged_slots.size() ||
            (same_identity && record.hook_count && context.position != *record.hook_count))
            throw std::logic_error("useState() hook count changed between renders");
        // The instance's key controls identity, not the implementation's root key.
        auto actual = result.props();
        actual.key = props.key;
        auto expanded = expand(Component(std::move(actual)),path + "/fn");
        if (!same_identity)
        {
            if (record.identity) retired_hooks_.emplace_back(path,std::move(record));
            record = {};
            record.identity = props.function_identity;
        }
        record.slots = std::move(staged_slots);
        record.hook_count = context.position;
        record.generation = generation_;
        return expanded;
    }

    auto actual = props;
    actual.children.clear();
    actual.children.reserve(props.children.size());
    for (std::size_t i = 0; i < props.children.size(); ++i)
    {
        const auto& child = props.children[i];
        const auto& key = child.props().key;
        const auto child_path = path + "/t" + std::to_string(static_cast<int>(props.type)) +
            (key.empty() ? "/i" + std::to_string(i) :
            "/k" + std::to_string(key.size()) + ":" + key);
        auto expanded_child = expand(child,child_path);
        if (expanded_child.props().type == Component::Type::Fragment)
        {
            // Splice descendants into the parent's layout without an extra RenderNode.
            for (const auto& grandchild : expanded_child.props().children)
                actual.children.push_back(grandchild);
        }
        else actual.children.push_back(std::move(expanded_child));
    }
    return Component(std::move(actual));
}

void Reconciler::reconcile(std::unique_ptr<RenderNode>& node, const Component& component)
{
    const auto& props = component.props();
    ++stats_.visited;
    bool created = false;
    if (!node || node->type_ != props.type || node->key_ != props.key)
    {
        if (node) ++stats_.removed;
        ++stats_.created;
        node.reset(new RenderNode(next_id_++,props.type,props.key));
        node->cursor_byte_ = props.text.size();
        created = true;
    }
    else ++stats_.reused;

    const bool message_changed = node->text_ != props.text;
    // Only the simple leaf and container types have a complete sizing dependency list.
    // For interactive widgets, conservatively recompute layout until their properties
    // expose a reliable geometry comparison.
    const bool predictable_layout = props.type == Component::Type::Text ||
        props.type == Component::Type::Column || props.type == Component::Type::Row ||
        props.type == Component::Type::Box || props.type == Component::Type::Grid ||
        props.type == Component::Type::Divider || props.type == Component::Type::Progress ||
        props.type == Component::Type::Slider;
    bool layout_dirty = created || !predictable_layout || message_changed ||
        node->style_ != props.layout || node->border_ != props.border ||
        node->columns_ != std::max(1,props.columns) ||
        node->progress_show_percentage_ != props.progress_show_percentage ||
        node->slider_show_value_ != props.slider_show_value;
    // Paint depends on visible properties as well as geometry. Callbacks and
    // event handlers do not affect pixels; interactive widgets remain dirty
    // until their visual state has a complete comparison strategy.
    bool paint_dirty = layout_dirty || node->theme_override_ != props.theme_override ||
        node->text_style_ != props.text_style ||
        node->background_ != props.background || node->surface_tone_ != props.surface_tone ||
        node->value_ != props.value || node->disabled_ != props.disabled ||
        node->progress_variant_ != props.progress_variant ||
        node->progress_indeterminate_ != props.progress_indeterminate ||
        node->slider_min_ != props.slider_min || node->slider_max_ != props.slider_max ||
        node->slider_step_ != props.slider_step;
    node->text_ = props.text;
    node->style_ = props.layout;
    node->text_style_ = props.text_style;
    node->theme_override_ = props.theme_override;
    node->border_ = props.border;
    node->background_ = props.background;
    node->button_variant_ = props.button_variant;
    node->button_size_ = props.button_size;
    node->checkbox_variant_ = props.checkbox_variant;
    node->text_field_variant_ = props.text_field_variant;
    node->scroll_offset_ = std::max(0,props.scroll_offset);
    node->on_scroll_ = props.on_scroll;
    node->scrollbar_visible_ = props.scrollbar_visible;
    node->value_ = props.value;
    node->progress_variant_ = props.progress_variant;
    node->progress_indeterminate_ = props.progress_indeterminate;
    node->progress_show_percentage_ = props.progress_show_percentage;
    node->slider_min_ = props.slider_min;
    node->slider_max_ = props.slider_max;
    node->slider_step_ = props.slider_step;
    node->slider_show_value_ = props.slider_show_value;
    node->slider_on_change_ = props.slider_on_change;
    node->columns_ = std::max(1,props.columns);
    node->surface_tone_ = props.surface_tone;
    node->on_click_ = props.on_click;
    node->on_toggle_ = props.on_toggle;
    node->on_event_capture_ = props.on_event_capture;
    node->on_event_ = props.on_event;
    node->on_change_ = props.on_change;
    node->placeholder_ = props.placeholder;
    node->disabled_ = props.disabled;
    node->select_options_ = props.select_options;
    node->radio_options_ = props.radio_options;
    node->tab_options_ = props.tab_options;
    node->table_columns_ = props.table_columns;
    node->table_rows_ = props.table_rows;
    node->table_filter_ = props.table_filter;
    node->table_format_cell_ = props.table_format_cell;
    node->tree_items_ = props.tree_items;
    node->tree_expanded_ = props.tree_expanded;
    node->tree_on_toggle_ = props.tree_on_toggle;
    node->split_direction_ = props.split_direction;
    node->split_position_ = props.split_position;
    node->split_min_first_ = std::max(0,props.split_min_first);
    node->split_min_second_ = std::max(0,props.split_min_second);
    node->split_on_change_ = props.split_on_change;
    if (node->tree_last_selected_ != props.text)
    {
        node->tree_last_selected_ = props.text;
        node->tree_pending_reveal_ = !props.text.empty();
    }
    node->tree_scroll_ = std::clamp(node->tree_scroll_,0,
        std::max(0,static_cast<int>(node->treeVisibleItems().size())-1));
    node->table_scroll_ = std::clamp(node->table_scroll_,0,
        std::max(0,static_cast<int>(node->tableOrder().size())-1));
    node->tab_panel_active_ = props.tab_panel_active;
    node->checked_ = props.checked;
    node->dialog_open_ = props.dialog_open;
    node->toast_duration_ms_ = std::max(0,props.toast_duration_ms);
    node->toast_severity_ = std::clamp(props.toast_severity,0,3);
    node->toast_on_close_ = props.toast_on_close;
    if (props.type == Component::Type::Toast)
    {
        if (props.toast_open && (!node->toast_requested_ || message_changed))
        {
            node->toast_visible_ = true;
            node->toast_started_ = std::chrono::steady_clock::now();
        }
        if (!props.toast_open) node->toast_visible_ = false;
        node->toast_requested_ = props.toast_open;
    }
    node->focus_scope_active_ = props.focus_scope_active;
    node->dialog_title_ = props.dialog_title;
    node->dialog_confirm_text_ = props.dialog_confirm_text;
    node->dialog_cancel_text_ = props.dialog_cancel_text;
    node->dialog_width_ = props.dialog_width;
    node->dialog_close_on_backdrop_ = props.dialog_close_on_backdrop;
    node->dialog_show_actions_ = props.dialog_show_actions;
    node->dialog_on_close_ = props.dialog_on_close;
    if (node->select_options_.empty() || node->disabled_) node->select_open_ = false;
    if (!node->select_options_.empty())
        node->select_highlight_ = std::clamp(node->select_highlight_,0,static_cast<int>(node->select_options_.size())-1);
    if (node->cursor_byte_ > node->text_.size()) node->cursor_byte_ = node->text_.size();
    while (node->cursor_byte_ > 0 && node->cursor_byte_ < node->text_.size() &&
        (static_cast<unsigned char>(node->text_[node->cursor_byte_]) & 0xC0) == 0x80)
        --node->cursor_byte_;
    auto previous = std::move(node->children_);
    if (previous.size() != props.children.size()) layout_dirty = paint_dirty = true;
    std::unordered_map<std::string,std::size_t> keyed;
    keyed.reserve(previous.size());
    for (std::size_t i = 0; i < previous.size(); ++i)
        if (previous[i] && !previous[i]->key_.empty())
            keyed.emplace(previous[i]->key_,i);

    std::vector<std::unique_ptr<RenderNode>> next;
    next.reserve(props.children.size());
    for (std::size_t i = 0; i < props.children.size(); ++i)
    {
        const auto& child = props.children[i];
        std::unique_ptr<RenderNode> matched;
        if (!child.props().key.empty())
        {
            const auto found = keyed.find(child.props().key);
            if (found != keyed.end())
            {
                if (found->second != i) layout_dirty = paint_dirty = true;
                matched = std::move(previous[found->second]);
            }
        }
        else if (i < previous.size() && previous[i] && previous[i]->key_.empty())
            matched = std::move(previous[i]);
        if (!matched) layout_dirty = paint_dirty = true;
        reconcile(matched,child);
        if (matched->layout_dirty_) layout_dirty = true;
        if (matched->paint_dirty_) paint_dirty = true;
        next.push_back(std::move(matched));
    }
    for (const auto& unused : previous)
        if (unused) ++stats_.removed;
    node->children_ = std::move(next);
    node->layout_dirty_ = layout_dirty;
    node->paint_dirty_ = paint_dirty || layout_dirty;
    if (node->layout_dirty_) ++stats_.layout_dirty;
    if (node->paint_dirty_) ++stats_.paint_dirty;
}
