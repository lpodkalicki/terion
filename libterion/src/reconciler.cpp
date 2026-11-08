/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/reconciler.h>
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
    validateKeys(component);
    stats_ = {};
    reconcile(root_,component);
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
    bool paint_dirty = layout_dirty || node->text_style_ != props.text_style ||
        node->background_ != props.background || node->surface_tone_ != props.surface_tone ||
        node->value_ != props.value || node->disabled_ != props.disabled ||
        node->progress_variant_ != props.progress_variant ||
        node->progress_indeterminate_ != props.progress_indeterminate ||
        node->slider_min_ != props.slider_min || node->slider_max_ != props.slider_max ||
        node->slider_step_ != props.slider_step;
    node->text_ = props.text;
    node->style_ = props.layout;
    node->text_style_ = props.text_style;
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
