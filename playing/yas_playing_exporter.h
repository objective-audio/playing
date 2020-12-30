//
//  yas_playing_exporter.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_result.h>
#include <cpp_utils/yas_task.h>
#include <playing/yas_playing_exporter_resource.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_timeline_container.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct exporter final {
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

    static exporter_ptr make_shared(std::string const &root_path, std::shared_ptr<task_queue> const &,
                                    task_priority const &, proc::sample_rate_t const);

   private:
    std::string const _root_path;
    std::shared_ptr<task_queue> const _queue;
    task_priority const _priority;
    chaining::value::holder_ptr<timeline_container_ptr> const _src_container;

    chaining::notifier_ptr<event> const _event_notifier = chaining::notifier<event>::make_shared();
    chaining::observer_pool _pool;

    exporter_resource_ptr const _resource = exporter_resource::make_shared();
    exporter_wptr _weak_exporter;

    exporter(std::string const &root_path, std::shared_ptr<task_queue> const &, task_priority const &,
             proc::sample_rate_t const);

    void _receive_timeline_event(proc::timeline::event_t const &event);
    void _receive_relayed_timeline_event(proc::timeline::relayed_event_t const &event);
    void _receive_relayed_track_event(proc::track::relayed_event_t const &event, proc::track_index_t const trk_idx);
    void _update_timeline(proc::timeline::track_map_t &&tracks);
    void _insert_tracks(proc::timeline::inserted_event_t const &event);
    void _erase_tracks(proc::timeline::erased_event_t const &event);
    void _insert_modules(proc::track_index_t const trk_idx, proc::track::inserted_event_t const &event);
    void _erase_modules(proc::track_index_t const trk_idx, proc::track::erased_event_t const &event);
    void _insert_module(proc::track_index_t const trk_idx, proc::time::range const range,
                        proc::module_vector::inserted_event_t const &event);
    void _erase_module(proc::track_index_t const trk_idx, proc::time::range const range,
                       proc::module_vector::erased_event_t const &event);
    void _push_export_task(proc::time::range const &range);

    void _export_fragments_on_task(exporter_resource_ptr const &, proc::time::range const &frags_range, task const &);
    [[nodiscard]] std::optional<error> _export_fragment_on_task(exporter_resource_ptr const &,
                                                                proc::time::range const &frag_range,
                                                                proc::stream const &stream);
    [[nodiscard]] std::optional<error> _remove_fragments_on_task(exporter_resource_ptr const &,
                                                                 proc::time::range const &frags_range, task const &);
    void _send_method_on_task(method const type, std::optional<proc::time::range> const &range);
    void _send_error_on_task(error const type, std::optional<proc::time::range> const &range);
    void _send_event_on_task(event event);
};
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::exporter::method const &);
std::string to_string(playing::exporter::error const &);
};  // namespace yas

std::ostream &operator<<(std::ostream &, yas::playing::exporter::method const &);
std::ostream &operator<<(std::ostream &, yas::playing::exporter::error const &);
