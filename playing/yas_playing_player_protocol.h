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

struct playable {
    using task_priority_t = player_task_priority;

    virtual ~playable() = default;

    virtual void set_identifier(std::string const &) = 0;
    virtual void set_channel_mapping(playing::channel_mapping const &) = 0;
    virtual void set_playing(bool const) = 0;
    virtual void seek(frame_index_t const) = 0;
    virtual void overwrite(std::optional<channel_index_t> const, fragment_range const) = 0;

    [[nodiscard]] virtual std::string const &identifier() const = 0;
    [[nodiscard]] virtual playing::channel_mapping channel_mapping() const = 0;
    [[nodiscard]] virtual bool is_playing() const = 0;
    [[nodiscard]] virtual frame_index_t current_frame() const = 0;

    [[nodiscard]] virtual observing::syncable observe_is_playing(std::function<void(bool const &)> &&) = 0;
};
}  // namespace yas::playing
