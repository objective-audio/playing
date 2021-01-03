//
//  yas_playing_player_utils.cpp
//

#include "yas_playing_player_utils.h"

#include <cpp_utils/yas_fast_each.h>

#include "yas_playing_math.h"

using namespace yas;
using namespace yas::playing;

proc::sample_rate_t player_utils::fragment_length(audio::format const &format) {
    return static_cast<proc::sample_rate_t>(format.sample_rate());
}

std::optional<fragment_index_t> player_utils::top_fragment_idx(proc::sample_rate_t const frag_length,
                                                               frame_index_t const frame) {
    if (frag_length > 0) {
        return math::floor_int(frame, frag_length) / frag_length;
    } else {
        return std::nullopt;
    }
}

std::vector<channel_index_t> player_utils::actually_ch_mapping(std::vector<channel_index_t> const &mapping,
                                                               std::size_t const ch_count) {
    std::vector<channel_index_t> mapped;

    auto each = make_fast_each(ch_count);
    while (yas_each_next(each)) {
        mapped.emplace_back(map_ch_idx(mapping, yas_each_index(each)));
    }

    return mapped;
}

channel_index_t player_utils::map_ch_idx(std::vector<channel_index_t> const &mapping, channel_index_t const ch_idx) {
    if (ch_idx < mapping.size()) {
        return mapping.at(ch_idx);
    } else {
        return ch_idx;
    }
}

uint32_t player_utils::process_length(frame_index_t const frame, frame_index_t const next_frame,
                                      uint32_t const frag_length) {
    return std::min(uint32_t(next_frame - frame), uint32_t(frag_length - math::mod_int(frame, frag_length)));
}

std::optional<fragment_index_t> player_utils::advancing_fragment_index(frame_index_t const frame,
                                                                       uint32_t const proc_length,
                                                                       uint32_t const frag_length) {
    if (math::mod_int(frame + proc_length, frag_length) == 0) {
#warning todo
        return frame / frag_length;
    } else {
        return std::nullopt;
    }
}
