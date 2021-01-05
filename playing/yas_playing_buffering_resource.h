//
//  yas_playing_buffering_resource.h
//

#pragma once

#include <playing/yas_playing_buffering_resource_protocol.h>
#include <playing/yas_playing_path.h>

namespace yas::playing {
struct buffering_resource final : buffering_resource_protocol {
    [[nodiscard]] setup_state_t setup_state() const override;
    [[nodiscard]] rendering_state_t rendering_state() const override;
    [[nodiscard]] std::size_t element_count() const override;
    [[nodiscard]] std::size_t channel_count_on_render() const override;
    [[nodiscard]] sample_rate_t fragment_length_on_render() const override;

    void set_creating_on_render(sample_rate_t const sample_rate, audio::pcm_format const &,
                                uint32_t const ch_count) override;
    [[nodiscard]] bool needs_create_on_render(sample_rate_t const sample_rate, audio::pcm_format const &,
                                              uint32_t const ch_count) override;
    void create_buffer_on_task() override;

    void set_all_writing_on_render(frame_index_t const, std::optional<channel_mapping_ptr> &&ch_mapping) override;
    void write_all_elements_on_task() override;
    void advance_on_render(fragment_index_t const) override;
    [[nodiscard]] bool write_elements_if_needed_on_task() override;
    void overwrite_element_on_render(element_address const &) override;

    [[nodiscard]] bool read_into_buffer_on_render(audio::pcm_buffer *, channel_index_t const,
                                                  frame_index_t const) override;

    using make_channel_f =
        std::function<buffering_channel_protocol_ptr(std::size_t const, audio::format const &, sample_rate_t const)>;

    static buffering_ptr make_shared(std::size_t const element_count, std::string const &root_path,
                                     std::string const &identifier, make_channel_f &&);

    frame_index_t all_writing_frame_for_test() const;
    channel_mapping_ptr const &ch_mapping_for_test() const;

   private:
    std::size_t const _element_count;
    std::string const _root_path;
    std::string const _identifier;
    make_channel_f const _make_channel_handler;

    std::atomic<setup_state_t> _setup_state{setup_state_t::initial};
    sample_rate_t _sample_rate = 0;
    sample_rate_t _frag_length = 0;
    audio::pcm_format _pcm_format;
    std::optional<audio::format> _format = std::nullopt;
    std::size_t _ch_count = 0;
    std::optional<path::timeline> _tl_path = std::nullopt;

    std::atomic<rendering_state_t> _rendering_state{rendering_state_t::waiting};
    frame_index_t _all_writing_frame = 0;
    channel_mapping_ptr _ch_mapping;

#warning channelsとch_mappingへのアクセスを検証
    std::vector<buffering_channel_protocol_ptr> _channels;

    buffering_resource(std::size_t const element_count, std::string const &root_path, std::string const &identifier,
                       make_channel_f &&);
};
}  // namespace yas::playing
