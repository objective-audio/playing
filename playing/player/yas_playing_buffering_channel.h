//
//  yas_playing_buffering_channel.h
//

#pragma once

#include <playing/yas_playing_buffering_channel_dependency.h>
#include <playing/yas_playing_buffering_resource_dependency.h>
#include <playing/yas_playing_ptr.h>

namespace yas::playing {
struct buffering_channel final : buffering_channel_interface {
    void write_all_elements_on_task(path::channel const &, fragment_index_t const top_frag_idx) override;
    [[nodiscard]] bool write_elements_if_needed_on_task() override;

    void advance_on_render(fragment_index_t const prev_frag_idx) override;
    void overwrite_element_on_render(fragment_range const) override;
    [[nodiscard]] bool read_into_buffer_on_render(audio::pcm_buffer *, frame_index_t const) override;

    [[nodiscard]] std::vector<std::shared_ptr<buffering_element_interface>> const &elements_for_test() const;

    [[nodiscard]] static buffering_channel_ptr make_shared(
        std::vector<std::shared_ptr<buffering_element_interface>> &&);

   private:
    std::vector<std::shared_ptr<buffering_element_interface>> const _elements;
    std::optional<path::channel> _ch_path = std::nullopt;

    explicit buffering_channel(std::vector<std::shared_ptr<buffering_element_interface>> &&);
};

[[nodiscard]] buffering_channel_ptr make_buffering_channel(std::size_t const element_count, audio::format const &format,
                                                           sample_rate_t const frag_length);
}  // namespace yas::playing
