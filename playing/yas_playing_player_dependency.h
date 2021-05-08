//
//  yas_playing_player_dependency.h
//

#pragma once

#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_renderer_types.h>

namespace yas::playing {
struct player_renderer_interface {
    virtual ~player_renderer_interface() = default;

    virtual void set_rendering_handler(renderer_rendering_f &&) = 0;
    virtual void set_is_rendering(bool const) = 0;
};

struct player_resource_interface {
    using overwrite_requests_t = std::vector<element_address>;
    using overwrite_requests_f = std::function<void(overwrite_requests_t const &)>;

    virtual ~player_resource_interface() = default;

    virtual reading_resource_protocol_ptr const &reading() const = 0;
    virtual std::shared_ptr<buffering_resource_interface> const &buffering() const = 0;

    virtual void set_playing_on_main(bool const) = 0;
    [[nodiscard]] virtual bool is_playing_on_render() const = 0;

    virtual void seek_on_main(frame_index_t const frame) = 0;
    [[nodiscard]] virtual std::optional<frame_index_t> pull_seek_frame_on_render() = 0;

    virtual void set_current_frame_on_render(frame_index_t const) = 0;
    [[nodiscard]] virtual frame_index_t current_frame() const = 0;

    virtual void add_overwrite_request_on_main(element_address &&) = 0;
    virtual void perform_overwrite_requests_on_render(overwrite_requests_f const &) = 0;
    virtual void reset_overwrite_requests_on_render() = 0;
};
}  // namespace yas::playing
