//
//  yas_playing_renderer_protocol.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <audio/yas_audio_ptr.h>
#include <chaining/yas_chaining_umbrella.h>
#include <playing/yas_playing_configulation.h>
#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_types.h>

#include <functional>

namespace yas::playing {
struct renderable {
    virtual ~renderable() = default;

    using rendering_f = std::function<void(audio::pcm_buffer *const)>;

    virtual void set_rendering_handler(rendering_f &&) = 0;
    virtual void set_is_rendering(bool const) = 0;
};

struct coordinator_renderable : renderable {
    [[nodiscard]] virtual sample_rate_t sample_rate() const = 0;
    [[nodiscard]] virtual audio::pcm_format pcm_format() const = 0;
    [[nodiscard]] virtual std::size_t channel_count() const = 0;

    [[nodiscard]] virtual chaining::chain_sync_t<configuration> configuration_chain() const = 0;
};
}  // namespace yas::playing
