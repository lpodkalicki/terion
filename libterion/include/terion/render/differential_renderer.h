/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_DIFFERENTIAL_RENDERER_H
#define TERION_RENDER_DIFFERENTIAL_RENDERER_H

#include <optional>
#include <terion/render/screen_buffer.h>
#include <terion/terminal.h>

namespace terion {

class DifferentialRenderer
{
public:
    void render(const ScreenBuffer& current, TerminalBackend& backend);
    void invalidate() { previous_.reset(); }

private:
    std::optional<ScreenBuffer> previous_;
}; /* End of class DifferentialRenderer */

} /* End of namespace terion */

#endif /* TERION_RENDER_DIFFERENTIAL_RENDERER_H */
