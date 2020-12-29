//
//  yas_playing_rendering_info.h
//

#pragma once

#include <playing/yas_playing_types.h>

#include <cstdint>
#include <optional>

namespace yas::playing {
struct rendering_info final {
    uint32_t const length;
    std::optional<fragment_index_t> const next_frag_idx;

    rendering_info(frame_index_t const current_frame, frame_index_t const next_frame, uint32_t const file_length);
};
}  // namespace yas::playing