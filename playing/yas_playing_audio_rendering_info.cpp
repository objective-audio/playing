//
//  yas_playing_audio_rendering_info.cpp
//

#include "yas_playing_audio_rendering_info.h"

#include <algorithm>

#include "yas_playing_math.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing {
static uint32_t processing_length(frame_index_t const frame, frame_index_t const next_frame,
                                  uint32_t const file_length) {
    return std::min(uint32_t(next_frame - frame), uint32_t(file_length - math::mod_int(frame, file_length)));
}

static std::optional<int64_t> get_next_file_idx(frame_index_t const frame, uint32_t const proc_length,
                                                uint32_t const file_length) {
    frame_index_t const next_frame = frame + proc_length;
    if (next_frame % file_length == 0) {
        return next_frame / file_length;
    } else {
        return std::nullopt;
    }
}
}  // namespace yas::playing

audio_rendering_info::audio_rendering_info(frame_index_t const frame, frame_index_t const next_frame,
                                           uint32_t const file_length)
    : length(processing_length(frame, next_frame, file_length)),
      next_frag_idx(get_next_file_idx(frame, this->length, file_length)) {
}
