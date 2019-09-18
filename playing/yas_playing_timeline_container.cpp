//
//  yas_playing_timeline_container.cpp
//

#include "yas_playing_timeline_container.h"

using namespace yas;
using namespace yas::playing;

struct timeline_container::impl {
    std::string const identifier;
    proc::sample_rate_t const sample_rate;
    std::optional<proc::timeline_ptr> const timeline;

    impl(std::string const &identifier, proc::sample_rate_t const sample_rate, proc::timeline_ptr const &timeline)
        : identifier(identifier), sample_rate(sample_rate), timeline(timeline) {
    }

    impl(proc::sample_rate_t const sample_rate) : sample_rate(sample_rate) {
    }
};

timeline_container::timeline_container(std::string const &identifier, proc::sample_rate_t const sample_rate,
                                       proc::timeline_ptr const &timeline)
    : _impl(std::make_shared<impl>(identifier, sample_rate, timeline)) {
}

timeline_container::timeline_container(proc::sample_rate_t const sample_rate)
    : _impl(std::make_shared<impl>(sample_rate)) {
}

std::string const &timeline_container::identifier() const {
    return this->_impl->identifier;
}

proc::sample_rate_t const &timeline_container::sample_rate() const {
    return this->_impl->sample_rate;
}

std::optional<proc::timeline_ptr> const &timeline_container::timeline() const {
    return this->_impl->timeline;
}

timeline_container_ptr timeline_container::make_shared(std::string const &identifier,
                                                       proc::sample_rate_t const sample_rate,
                                                       proc::timeline_ptr const &timeline) {
    return timeline_container_ptr(new timeline_container{identifier, sample_rate, timeline});
}

timeline_container_ptr timeline_container::make_shared(proc::sample_rate_t const sample_rate) {
    return timeline_container_ptr(new timeline_container{sample_rate});
}
