//
//  yas_playing_timeline_canceling.cpp
//

#include "yas_playing_timeline_canceling.h"

using namespace yas;
using namespace yas::playing;

#pragma mark - cancel_id

timeline_cancel_matcher::timeline_cancel_matcher(std::optional<proc::time::range> const &range) : range(range) {
}

bool timeline_cancel_matcher::is_equal(task_cancel_id_ptr const &rhs) const {
    if (auto casted_rhs = std::dynamic_pointer_cast<timeline_cancel_request>(rhs)) {
        return casted_rhs->is_match(this->_weak_matcher.lock());
    }
    return false;
}

timeline_cancel_matcher_ptr timeline_cancel_matcher::make_shared() {
    return make_shared(std::nullopt);
}

timeline_cancel_matcher_ptr timeline_cancel_matcher::make_shared(std::optional<proc::time::range> const &range) {
    auto shared = timeline_cancel_matcher_ptr(new timeline_cancel_matcher{range});
    shared->_weak_matcher = shared;
    return shared;
}

#pragma mark - range_cancel_request

timeline_range_cancel_request::timeline_range_cancel_request(proc::time::range const &range) : range(range) {
}

bool timeline_range_cancel_request::is_match(timeline_cancel_matcher_ptr const &matcher) const {
    if (matcher->range.has_value()) {
        return this->range.is_contain(*matcher->range);
    } else {
        return false;
    }
}

bool timeline_range_cancel_request::is_equal(task_cancel_id_ptr const &rhs) const {
    if (auto casted_rhs = std::dynamic_pointer_cast<timeline_cancel_matcher>(rhs)) {
        return this->is_match(casted_rhs);
    }
    return false;
}

timeline_range_cancel_request_ptr timeline_range_cancel_request::make_shared(proc::time::range const &range) {
    return timeline_range_cancel_request_ptr(new timeline_range_cancel_request{range});
}
