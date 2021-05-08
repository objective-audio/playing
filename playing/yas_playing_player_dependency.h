//
//  yas_playing_player_dependency.h
//

#pragma once

#include <playing/yas_playing_renderer_types.h>

namespace yas::playing {
struct player_renderer_interface {
    virtual ~player_renderer_interface() = default;

    virtual void set_rendering_handler(renderer_rendering_f &&) = 0;
    virtual void set_is_rendering(bool const) = 0;
};
}  // namespace yas::playing
