//
//  yas_playing_coordinator_dependency.h
//

#pragma once

#include <observing/yas_observing_umbrella.h>
#include <playing/yas_playing_renderer_types.h>

namespace yas::playing {
struct coordinator_renderer_interface {
    virtual ~coordinator_renderer_interface() = default;

    virtual void set_rendering_sample_rate(sample_rate_t const) = 0;
    virtual void set_rendering_pcm_format(audio::pcm_format const) = 0;

    [[nodiscard]] virtual renderer_format const &format() const = 0;

    using renderer_format_observing_handler_f = std::function<void(renderer_format const &)>;

    [[nodiscard]] virtual observing::syncable observe_format(renderer_format_observing_handler_f &&) = 0;
};
}  // namespace yas::playing
