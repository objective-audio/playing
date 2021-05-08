//
//  yas_playing_timeline_canceling.h
//

#pragma once

#include <cpp_utils/yas_task_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_umbrella.h>

namespace yas::playing {
struct timeline_cancel_matcher final : task_cancel_id {
    std::optional<proc::time::range> const range;

    static timeline_cancel_matcher_ptr make_shared();
    static timeline_cancel_matcher_ptr make_shared(std::optional<proc::time::range> const &);

   private:
    std::weak_ptr<timeline_cancel_matcher> _weak_matcher;

    timeline_cancel_matcher(std::optional<proc::time::range> const &);

    bool is_equal(task_cancel_id_ptr const &) const override;
};

struct timeline_cancel_request {
    virtual ~timeline_cancel_request() = default;

    virtual bool is_match(timeline_cancel_matcher_ptr const &) const = 0;
};

// requestの範囲に完全に含まれていたらキャンセルさせる
struct timeline_range_cancel_request final : timeline_cancel_request, task_cancel_id {
    proc::time::range const range;

    static timeline_range_cancel_request_ptr make_shared(proc::time::range const &);

   private:
    explicit timeline_range_cancel_request(proc::time::range const &range);

    bool is_match(timeline_cancel_matcher_ptr const &) const override;
    bool is_equal(task_cancel_id_ptr const &) const override;
};
}  // namespace yas::playing
