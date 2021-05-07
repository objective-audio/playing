//
//  yas_playing_renderer_protocol.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <audio/yas_audio_ptr.h>
#include <observing/yas_observing_umbrella.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_renderer_types.h>
#include <processing/yas_processing_common_types.h>

#include <functional>

namespace yas::playing {
struct renderable {
    virtual ~renderable() = default;

    using rendering_f = std::function<void(audio::pcm_buffer *const)>;

    virtual void set_rendering_handler(rendering_f &&) = 0;
    virtual void set_is_rendering(bool const) = 0;
};

struct coordinator_renderable : renderable {
    virtual void set_rendering_sample_rate(sample_rate_t const) = 0;
    virtual void set_rendering_pcm_format(audio::pcm_format const) = 0;

    [[nodiscard]] virtual renderer_format const &format() const = 0;

    using renderer_format_observing_handler_f = std::function<void(renderer_format const &)>;

    [[nodiscard]] virtual observing::syncable observe_format(renderer_format_observing_handler_f &&) = 0;
};
}  // namespace yas::playing
