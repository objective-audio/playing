//
//  yas_playing_timeline_container.cpp
//

#include "yas_playing_timeline_container.h"

using namespace yas;
using namespace yas::playing;

timeline_container::timeline_container(std::string const &identifier, proc::sample_rate_t const sample_rate,
                                       proc::timeline_ptr const &timeline)
    : _identifier(identifier), _sample_rate(sample_rate), _timeline(timeline) {
}

timeline_container::timeline_container(proc::sample_rate_t const sample_rate) : _sample_rate(sample_rate) {
}

std::string const &timeline_container::identifier() const {
    return this->_identifier;
}

proc::sample_rate_t const &timeline_container::sample_rate() const {
    return this->_sample_rate;
}

std::optional<proc::timeline_ptr> const &timeline_container::timeline() const {
    return this->_timeline;
}

timeline_container_ptr timeline_container::make_shared(std::string const &identifier,
                                                       proc::sample_rate_t const sample_rate,
                                                       proc::timeline_ptr const &timeline) {
    return timeline_container_ptr(new timeline_container{identifier, sample_rate, timeline});
}

timeline_container_ptr timeline_container::make_shared(proc::sample_rate_t const sample_rate) {
    return timeline_container_ptr(new timeline_container{sample_rate});
}
