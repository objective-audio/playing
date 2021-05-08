//
//  yas_playing_player_resource.h
//

#pragma once

#include <playing/yas_playing_player_dependency.h>
#include <playing/yas_playing_ptr.h>

#include <mutex>

namespace yas::playing {
struct player_resource final : player_resource_interface {
    reading_resource_protocol_ptr const &reading() const override;
    buffering_resource_protocol_ptr const &buffering() const override;

    void set_playing_on_main(bool const) override;
    [[nodiscard]] bool is_playing_on_render() const override;

    void seek_on_main(frame_index_t const frame) override;
    [[nodiscard]] std::optional<frame_index_t> pull_seek_frame_on_render() override;

    void set_current_frame_on_render(frame_index_t const) override;
    [[nodiscard]] frame_index_t current_frame() const override;

    void add_overwrite_request_on_main(element_address &&) override;
    void perform_overwrite_requests_on_render(overwrite_requests_f const &) override;
    void reset_overwrite_requests_on_render() override;

    static player_resource_ptr make_shared(reading_resource_protocol_ptr const &,
                                           buffering_resource_protocol_ptr const &);

   private:
    reading_resource_protocol_ptr const _reading;
    buffering_resource_protocol_ptr const _buffering;

    std::atomic<bool> _is_playing{false};
    std::atomic<frame_index_t> _current_frame{0};

    std::mutex _seek_mutex;
    std::optional<frame_index_t> _seek_frame = std::nullopt;

    std::mutex _overwrite_mutex;
    overwrite_requests_t _overwrite_requests;
    bool _is_overwritten = false;

    player_resource(reading_resource_protocol_ptr const &, buffering_resource_protocol_ptr const &);
};
}  // namespace yas::playing
