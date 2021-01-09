//
//  yas_playing_player_protocol.h
//

#pragma once

#include <playing/yas_playing_renderer_protocol.h>
#include <playing/yas_playing_types.h>

namespace yas::playing {
struct player_task_priority final {
    uint32_t setup = 0;
    uint32_t rendering = 1;
};

struct player_protocol {
    using task_priority_t = player_task_priority;

    virtual ~player_protocol() = default;

    virtual void set_channel_mapping(channel_mapping_ptr const &) = 0;
    virtual void set_playing(bool const) = 0;
    virtual void seek(frame_index_t const) = 0;
    virtual void overwrite(std::optional<channel_index_t> const, fragment_index_t const) = 0;

    [[nodiscard]] virtual channel_mapping_ptr const &channel_mapping() const = 0;
    [[nodiscard]] virtual bool is_playing() const = 0;
    [[nodiscard]] virtual frame_index_t current_frame() const = 0;

    [[nodiscard]] virtual chaining::chain_sync_t<bool> is_playing_chain() const = 0;
};
}  // namespace yas::playing
