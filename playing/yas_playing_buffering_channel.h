//
//  yas_playing_buffering_channel.h
//

#pragma once

#include <playing/yas_playing_buffering_channel_protocol.h>
#include <playing/yas_playing_ptr.h>

namespace yas::playing {
struct buffering_channel final : buffering_channel_protocol {
    void write_all_elements_on_task(path::channel const &, fragment_index_t const top_frag_idx) override;
    [[nodiscard]] bool write_elements_if_needed_on_task() override;

    void advance_on_render(fragment_index_t const prev_frag_idx, fragment_index_t const new_frag_idx) override;
    void overwrite_element_on_render(fragment_index_t const) override;
    [[nodiscard]] bool read_into_buffer_on_render(audio::pcm_buffer *, frame_index_t const) override;

    [[nodiscard]] std::vector<buffering_element_protocol_ptr> const &elements_for_test() const;

    [[nodiscard]] static buffering_channel_ptr make_shared(std::vector<buffering_element_protocol_ptr> &&);

   private:
    std::vector<buffering_element_protocol_ptr> _elements;
    std::optional<path::channel> _ch_path = std::nullopt;

    explicit buffering_channel(std::vector<buffering_element_protocol_ptr> &&);
};

[[nodiscard]] buffering_channel_ptr make_buffering_channel(std::size_t const element_count, audio::format const &format,
                                                           sample_rate_t const frag_length);
}  // namespace yas::playing
