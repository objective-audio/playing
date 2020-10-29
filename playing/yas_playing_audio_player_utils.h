//
//  yas_playing_audio_player_utils.h
//

#pragma once

#include <audio/yas_audio_format.h>
#include <playing/yas_playing_types.h>

namespace yas::playing::audio_player_utils {
proc::sample_rate_t fragment_length(audio::format const &);
std::optional<playing::fragment_index_t> top_fragment_idx(proc::sample_rate_t const frag_length, frame_index_t const);

std::vector<playing::channel_index_t> actually_ch_mapping(std::vector<channel_index_t> const &mapping,
                                                          std::size_t const ch_count);
playing::channel_index_t map_ch_idx(std::vector<channel_index_t> const &mapping, channel_index_t const);
}  // namespace yas::playing::audio_player_utils
