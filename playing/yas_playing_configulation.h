//
//  yas_playing_configulation.h
//

#pragma once

#include <playing/yas_playing_types.h>
#include <processing/yas_processing_types.h>

namespace yas::playing {
struct configuration final {
    sample_rate_t sample_rate;
    audio::pcm_format pcm_format = audio::pcm_format::float32;
    std::size_t channel_count;

    bool operator==(configuration const &rhs) const {
        return this->sample_rate == rhs.sample_rate && this->pcm_format == rhs.pcm_format &&
               this->channel_count == rhs.channel_count;
    }

    bool operator!=(configuration const &rhs) const {
        return !(*this == rhs);
    }
};
}  // namespace yas::playing
