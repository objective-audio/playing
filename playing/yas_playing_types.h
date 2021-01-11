//
//  yas_playing_types.h
//

#pragma once

#include <processing/yas_processing_types.h>

namespace yas::playing {
using channel_index_t = proc::channel_index_t;
using fragment_index_t = int64_t;
using track_index_t = proc::track_index_t;
using frame_index_t = proc::frame_index_t;
using module_index_t = proc::module_index_t;
using length_t = proc::length_t;
using sample_rate_t = proc::sample_rate_t;

struct fragment_range {
    fragment_index_t index;
    length_t length;
};

struct element_address {
    std::optional<channel_index_t> file_channel_index;  // nulloptは全ch
    fragment_range fragment_range;
};

enum class sample_store_type : char {
    unknown = 0,
    float64 = 1,
    float32 = 2,
    int64 = 3,
    uint64 = 4,
    int32 = 5,
    uint32 = 6,
    int16 = 7,
    uint16 = 8,
    int8 = 9,
    uint8 = 10,
    boolean = 11,
};
}  // namespace yas::playing
