//
//  yas_playing_audio_rendering.h
//

#pragma once

#include <playing/yas_playing_audio_rendering_protocol.h>
#include <playing/yas_playing_ptr.h>

#include <mutex>

namespace yas::playing {
struct audio_rendering : audio_rendering_protocol {
    void set_is_playing_on_main(bool const) override;
    [[nodiscard]] bool is_playing_on_render() const override;

    void seek_on_main(frame_index_t const frame) override;
    [[nodiscard]] std::optional<frame_index_t> pull_seek_frame_on_render() override;

    void set_ch_mapping_on_main(std::vector<channel_index_t> const &ch_mapping) override;
    [[nodiscard]] std::optional<std::vector<channel_index_t>> pull_ch_mapping_on_render() override;

    void set_play_frame_on_render(frame_index_t const) override;
    [[nodiscard]] frame_index_t play_frame() const override;

    void add_overwrite_request_on_main(element_address &&) override;
    void perform_overwrite_requests_on_render(overwrite_requests_f const &) override;
    void reset_overwrite_requests_on_render() override;

    static audio_rendering_ptr make_shared();

   private:
    std::atomic<bool> _is_playing{false};
    std::atomic<frame_index_t> _play_frame{0};

    std::recursive_mutex _seek_mutex;
    std::optional<frame_index_t> _seek_frame = std::nullopt;

    std::recursive_mutex _ch_mapping_mutex;
    std::vector<channel_index_t> _ch_mapping;
    bool _ch_mapping_changed = false;

    std::recursive_mutex _overwrite_mutex;
    overwrite_requests_t _overwrite_requests;
    bool _is_overwritten = false;

    audio_rendering();
};
}  // namespace yas::playing
