//
//  yas_playing_audio_renderer_protocol.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <audio/yas_audio_ptr.h>
#include <chaining/yas_chaining_umbrella.h>
#include <playing/yas_playing_audio_configulation.h>
#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_types.h>

#include <functional>

namespace yas::playing {
struct audio_renderable {
    virtual ~audio_renderable() = default;

    using rendering_f = std::function<void(audio::pcm_buffer *const)>;

    virtual void set_rendering_handler(rendering_f &&) = 0;
    virtual void set_is_rendering(bool const) = 0;

    [[nodiscard]] virtual proc::sample_rate_t sample_rate() const = 0;
    [[nodiscard]] virtual audio::pcm_format pcm_format() const = 0;
    [[nodiscard]] virtual std::size_t channel_count() const = 0;

    [[nodiscard]] virtual chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() = 0;
    [[nodiscard]] virtual chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() = 0;
    [[nodiscard]] virtual chaining::chain_sync_t<std::size_t> chain_channel_count() = 0;

    audio_renderable_ptr cast(audio_renderable_ptr const &renderable) {
        return renderable;
    }
};

struct audio_coordinator_renderable : audio_renderable {
    [[nodiscard]] virtual chaining::chain_sync_t<audio_configuration> configuration_chain() const = 0;
};
}  // namespace yas::playing
