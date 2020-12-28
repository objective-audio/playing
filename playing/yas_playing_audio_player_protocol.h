//
//  yas_playing_audio_player_protocol.h
//

#pragma once

#include <playing/yas_playing_audio_renderer_protocol.h>
#include <playing/yas_playing_types.h>

namespace yas::playing {
struct audio_player_task_priority {
    uint32_t setup;
    uint32_t rendering;
};

struct audio_playable {
    using task_priority_t = audio_player_task_priority;

    virtual ~audio_playable() = default;

    virtual void set_channel_mapping(channel_mapping_ptr const &) = 0;
    virtual void set_playing(bool const) = 0;
    virtual void seek(frame_index_t const) = 0;
    virtual void overwrite(channel_index_t const, fragment_index_t const) = 0;

    [[nodiscard]] virtual channel_mapping_ptr const &channel_mapping() const = 0;
    [[nodiscard]] virtual bool is_playing() const = 0;
    [[nodiscard]] virtual frame_index_t current_frame() const = 0;

    [[nodiscard]] virtual chaining::chain_sync_t<bool> is_playing_chain() const = 0;
};
}  // namespace yas::playing
