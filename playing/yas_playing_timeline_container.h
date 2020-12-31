//
//  yas_playing_timeline_container.h
//

#pragma once

#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct timeline_container final {
    std::string const &identifier() const;
    proc::sample_rate_t const &sample_rate() const;
    std::optional<proc::timeline_ptr> const &timeline() const;

    bool is_available() const;

    static timeline_container_ptr make_shared(std::string const &identifier, proc::sample_rate_t const sample_rate,
                                              std::optional<proc::timeline_ptr> const &timeline);
    static timeline_container_ptr make_shared_empty();

   private:
    std::string const _identifier;
    proc::sample_rate_t const _sample_rate;
    std::optional<proc::timeline_ptr> const _timeline;

    timeline_container(std::string const &identifier, sample_rate_t const sample_rate,
                       std::optional<proc::timeline_ptr> const &timeline);
};
}  // namespace yas::playing
