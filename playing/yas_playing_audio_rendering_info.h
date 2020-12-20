//
//  yas_playing_audio_rendering_info.h
//

#pragma once

#include <playing/yas_playing_types.h>

#include <cstdint>
#include <optional>

namespace yas::playing {
struct audio_rendering_info {
    uint32_t const length;
    std::optional<fragment_index_t> const next_frag_idx;

    audio_rendering_info(frame_index_t const play_frame, frame_index_t const next_frame, uint32_t const file_length);
};
}  // namespace yas::playing
