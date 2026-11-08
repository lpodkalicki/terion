/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/reconciler.h>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>

using namespace terion;

namespace {

void validateKeys(const Component& component)
{
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
    reconcile(root_,component);
}

void Reconciler::reconcile(std::unique_ptr<RenderNode>& node, const Component& component)
{
    const auto& props = component.props();
    if (!node || node->type_ != props.type || node->key_ != props.key)
    {
        node.reset(new RenderNode(next_id_++,props.type,props.key));
        node->cursor_byte_ = props.text.size();
    }

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
    node->tree_scroll_ = std::clamp(node->tree_scroll_,0,
        std::max(0,static_cast<int>(node->treeVisibleItems().size())-1));
    node->table_scroll_ = std::clamp(node->table_scroll_,0,
        std::max(0,static_cast<int>(node->tableOrder().size())-1));
    node->tab_panel_active_ = props.tab_panel_active;
    node->checked_ = props.checked;
    node->dialog_open_ = props.dialog_open;
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
    node->children_.reserve(props.children.size());

    for (const auto& child : props.children)
    {
        std::unique_ptr<RenderNode> matched;
        for (auto& candidate : previous)
        {
            if (!candidate) continue;
            if (candidate->key_ == child.props().key)
            {
                matched = std::move(candidate);
                break;
            }
        }
        reconcile(matched,child);
        node->children_.push_back(std::move(matched));
    }
}
