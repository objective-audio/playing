//
//  yas_playing_timeline_container.h
//

#pragma once

#include <processing/yas_processing_timeline.h>
#include "yas_playing_ptr.h"

namespace yas::playing {
struct timeline_container final {
    std::string const &identifier() const;
    proc::sample_rate_t const &sample_rate() const;
    std::optional<proc::timeline_ptr> const &timeline() const;

    static timeline_container_ptr make_shared(std::string const &identifier, proc::sample_rate_t const sample_rate,
                                              proc::timeline_ptr const &timeline);
    static timeline_container_ptr make_shared(proc::sample_rate_t const sample_rate);

   private:
    class impl;

    std::shared_ptr<impl> _impl;

    timeline_container(std::string const &identifier, proc::sample_rate_t const sample_rate,
                       proc::timeline_ptr const &timeline);
    timeline_container(proc::sample_rate_t const sample_rate);
};
}  // namespace yas::playing
