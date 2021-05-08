//
//  yas_playing_player_protocol.h
//

#pragma once

#include <observing/yas_observing_umbrella.h>
#include <playing/yas_playing_channel_mapping.h>
#include <playing/yas_playing_types.h>

namespace yas::playing {
struct player_task_priority final {
    uint32_t setup = 0;
    uint32_t rendering = 1;
};
}  // namespace yas::playing
