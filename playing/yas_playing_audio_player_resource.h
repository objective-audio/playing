//
//  yas_playing_audio_player_resource.h
//

#pragma once

#include <playing/yas_playing_audio_player_resource_protocol.h>
#include <playing/yas_playing_ptr.h>

#include <mutex>

namespace yas::playing {
struct audio_player_resource : audio_player_resource_protocol {
    audio_reading_protocol_ptr const &reading() const override;
    audio_buffering_protocol_ptr const &buffering() const override;

    void set_playing_on_main(bool const) override;
    [[nodiscard]] bool is_playing_on_render() const override;

    void seek_on_main(frame_index_t const frame) override;
    [[nodiscard]] std::optional<frame_index_t> pull_seek_frame_on_render() override;

    void set_channel_mapping_on_main(std::vector<channel_index_t> const &ch_mapping) override;
    [[nodiscard]] std::optional<std::vector<channel_index_t>> pull_channel_mapping_on_render() override;

    void set_current_frame_on_render(frame_index_t const) override;
    [[nodiscard]] frame_index_t current_frame() const override;

    void add_overwrite_request_on_main(element_address &&) override;
    void perform_overwrite_requests_on_render(overwrite_requests_f const &) override;
    void reset_overwrite_requests_on_render() override;

    static audio_player_resource_ptr make_shared(audio_reading_ptr const &, audio_buffering_ptr const &);

   private:
    audio_reading_protocol_ptr const _reading;
    audio_buffering_protocol_ptr const _buffering;

    std::atomic<bool> _is_playing{false};
    std::atomic<frame_index_t> _current_frame{0};

    std::recursive_mutex _seek_mutex;
    std::optional<frame_index_t> _seek_frame = std::nullopt;

    std::recursive_mutex _ch_mapping_mutex;
    std::vector<channel_index_t> _ch_mapping;
    bool _ch_mapping_changed = false;

    std::recursive_mutex _overwrite_mutex;
    overwrite_requests_t _overwrite_requests;
    bool _is_overwritten = false;

    audio_player_resource(audio_reading_ptr const &, audio_buffering_ptr const &);
};
}  // namespace yas::playing
