//
//  yas_playing_player_resource_dependency.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <audio/yas_audio_types.h>
#include <playing/yas_playing_buffering_resource_protocol.h>
#include <playing/yas_playing_channel_mapping.h>
#include <playing/yas_playing_reading_resource_protocol.h>

namespace yas::playing {
struct reading_resource_interface {
    using state_t = reading_resource_state;

    virtual ~reading_resource_interface() = default;

    [[nodiscard]] virtual state_t state() const = 0;
    [[nodiscard]] virtual audio::pcm_buffer *buffer_on_render() = 0;

    [[nodiscard]] virtual bool needs_create_on_render(sample_rate_t const sample_rate, audio::pcm_format const,
                                                      uint32_t const length) const = 0;
    virtual void set_creating_on_render(sample_rate_t const sample_rate, audio::pcm_format const,
                                        uint32_t const length) = 0;
    virtual void create_buffer_on_task() = 0;
};

struct buffering_resource_interface {
    using setup_state_t = audio_buffering_setup_state;
    using rendering_state_t = audio_buffering_rendering_state;

    virtual ~buffering_resource_interface() = default;

    [[nodiscard]] virtual setup_state_t setup_state() const = 0;
    [[nodiscard]] virtual rendering_state_t rendering_state() const = 0;
    [[nodiscard]] virtual std::size_t element_count() const = 0;
    [[nodiscard]] virtual std::size_t channel_count_on_render() const = 0;
    [[nodiscard]] virtual sample_rate_t fragment_length_on_render() const = 0;

    virtual void set_creating_on_render(sample_rate_t const sample_rate, audio::pcm_format const &,
                                        uint32_t const ch_count) = 0;
    [[nodiscard]] virtual bool needs_create_on_render(sample_rate_t const sample_rate, audio::pcm_format const &,
                                                      uint32_t const ch_count) = 0;

    virtual void create_buffer_on_task() = 0;

    virtual void set_all_writing_on_render(frame_index_t const) = 0;
    virtual void write_all_elements_on_task() = 0;
    virtual void advance_on_render(fragment_index_t const) = 0;
    [[nodiscard]] virtual bool write_elements_if_needed_on_task() = 0;
    virtual void overwrite_element_on_render(element_address const &) = 0;

    virtual bool needs_all_writing_on_render() const = 0;
    virtual void set_channel_mapping_request_on_main(channel_mapping const &) = 0;
    virtual void set_identifier_request_on_main(std::string const &) = 0;

    [[nodiscard]] virtual bool read_into_buffer_on_render(audio::pcm_buffer *, channel_index_t const,
                                                          frame_index_t const) = 0;
};
}  // namespace yas::playing
