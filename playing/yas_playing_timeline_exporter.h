//
//  yas_playing_timeline_exporter.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_result.h>
#include <cpp_utils/yas_task.h>
#include <processing/yas_processing_timeline.h>
#include "yas_playing_ptr.h"
#include "yas_playing_timeline_container.h"

namespace yas::playing {
struct timeline_exporter {
    enum class method {
        reset,
        export_began,
        export_ended,
    };

    enum class error {
        remove_fragment_failed,
        create_directory_failed,
        write_signal_failed,
        write_numbers_failed,
        get_content_paths_failed,
    };

    using result_t = result<method, error>;

    struct event {
        result_t const result;
        std::optional<proc::time::range> const range;
    };

    struct task_priority {
        task_priority_t const timeline;
        task_priority_t const fragment;
    };

    void set_timeline_container(timeline_container_ptr const &);

    chaining::chain_unsync_t<event> event_chain() const;

    static timeline_exporter_ptr make_shared(std::string const &root_path, std::shared_ptr<task_queue> const &,
                                             task_priority const &, proc::sample_rate_t const);

   private:
    class impl;

    std::shared_ptr<impl> _impl;

    timeline_exporter(std::string const &root_path, std::shared_ptr<task_queue> const &, task_priority const &,
                      proc::sample_rate_t const);

    void _prepare(timeline_exporter_ptr const &);
};
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::timeline_exporter::method const &);
std::string to_string(playing::timeline_exporter::error const &);
};  // namespace yas

std::ostream &operator<<(std::ostream &, yas::playing::timeline_exporter::method const &);
std::ostream &operator<<(std::ostream &, yas::playing::timeline_exporter::error const &);
