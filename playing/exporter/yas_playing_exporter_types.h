//
//  yas_playing_exporter_types.h
//

#pragma once

#include <cpp_utils/yas_result.h>
#include <cpp_utils/yas_task_queue.h>
#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
enum class exporter_method {
    reset,
    export_began,
    export_ended,
};

enum class exporter_error {
    remove_fragment_failed,
    create_directory_failed,
    write_signal_failed,
    write_numbers_failed,
    get_content_paths_failed,
};

using exporter_result_t = result<exporter_method, exporter_error>;

struct exporter_event final {
    exporter_result_t const result;
    std::optional<proc::time::range> const range;
};

struct exporter_task_priority final {
    task_priority_t const timeline;
    task_priority_t const fragment;
};

using exporter_task = task<timeline_cancel_matcher_ptr>;
using exporter_task_queue = task_queue<timeline_cancel_matcher_ptr>;
}  // namespace yas::playing
