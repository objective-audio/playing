//
//  yas_playing_renderer_types.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <audio/yas_audio_ptr.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_common_types.h>

namespace yas::playing {
using renderer_rendering_f = std::function<void(audio::pcm_buffer *const)>;

struct renderer_format final {
    sample_rate_t sample_rate;
    audio::pcm_format pcm_format = audio::pcm_format::float32;
    std::size_t channel_count;

    bool operator==(renderer_format const &rhs) const {
        return this->sample_rate == rhs.sample_rate && this->pcm_format == rhs.pcm_format &&
               this->channel_count == rhs.channel_count;
    }

    bool operator!=(renderer_format const &rhs) const {
        return !(*this == rhs);
    }
};
}  // namespace yas::playing
