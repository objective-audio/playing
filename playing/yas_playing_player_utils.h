//
//  yas_playing_player_utils.h
//

#pragma once

#include <audio/yas_audio_format.h>
#include <playing/yas_playing_types.h>

namespace yas::playing::player_utils {
proc::sample_rate_t fragment_length(audio::format const &);
std::optional<fragment_index_t> top_fragment_idx(proc::sample_rate_t const frag_length, frame_index_t const);

uint32_t process_length(frame_index_t const frame, frame_index_t const next_frame, uint32_t const frag_length);
std::optional<fragment_index_t> advancing_fragment_index(frame_index_t const frame, uint32_t const proc_length,
                                                         uint32_t const frag_length);
}  // namespace yas::playing::player_utils
