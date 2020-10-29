//
//  yas_playing_audio_rendering_protocol.h
//

#pragma once

#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>

#include <functional>
#include <vector>

namespace yas::playing {
struct audio_rendering_protocol {
    using overwrite_requests_t = std::vector<element_address>;
    using overwrite_requests_f = std::function<void(const overwrite_requests_t &)>;

    virtual ~audio_rendering_protocol() = default;

    virtual void set_is_playing_on_main(bool const) = 0;
    [[nodiscard]] virtual bool is_playing_on_render() const = 0;

    virtual void seek_on_main(frame_index_t const frame) = 0;
    [[nodiscard]] virtual std::optional<frame_index_t> pull_seek_frame_on_render() = 0;

    virtual void set_ch_mapping_on_main(std::vector<channel_index_t> const &ch_mapping) = 0;
    [[nodiscard]] virtual std::optional<std::vector<channel_index_t>> pull_ch_mapping_on_render() = 0;

    virtual void set_play_frame_on_render(frame_index_t const) = 0;
    [[nodiscard]] virtual frame_index_t play_frame_on_main() const = 0;
    [[nodiscard]] virtual frame_index_t play_frame_on_render() const = 0;

    virtual void add_overwrite_request_on_main(element_address &&) = 0;
    virtual void perform_overwrite_requests_on_render(overwrite_requests_f const &) = 0;
    virtual void reset_overwrite_requests_on_render() = 0;
};
}  // namespace yas::playing
