/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/component.h>

using namespace terion;

Component Component::text(std::string value, std::string key)
{
    return Component({.type = Type::Text,.key = std::move(key),.text = std::move(value),.children = {}});
}

Component Component::column(std::vector<Component> children, std::string key)
{
    return Component({.type = Type::Column,.key = std::move(key),.text = {},.children = std::move(children)});
}
