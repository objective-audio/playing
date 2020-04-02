//
//  yas_playing_audio_player_protocol.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <audio/yas_audio_ptr.h>
#include <chaining/yas_chaining_umbrella.h>
#include <processing/yas_processing_types.h>

#include <functional>

#include "yas_playing_ptr.h"

namespace yas::playing {
struct audio_renderable {
    using rendering_f = std::function<void(audio::pcm_buffer_ptr const &)>;

    virtual void set_rendering_handler(rendering_f) = 0;
    virtual chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() = 0;
    virtual chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() = 0;
    virtual chaining::chain_sync_t<std::size_t> chain_channel_count() = 0;
    virtual void set_is_rendering(bool const) = 0;

    audio_renderable_ptr cast(audio_renderable_ptr const &renderable) {
        return renderable;
    }
};
}  // namespace yas::playing
