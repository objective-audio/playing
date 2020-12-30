//
//  yas_playing_timeline_exporter.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_result.h>
#include <cpp_utils/yas_task.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_timeline_container.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct timeline_exporter final {
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
    std::string const _root_path;
    std::shared_ptr<task_queue> const _queue;
    task_priority const _priority;
    chaining::value::holder_ptr<timeline_container_ptr> const _src_container;

    chaining::notifier_ptr<event> const _event_notifier = chaining::notifier<event>::make_shared();
    chaining::observer_pool _pool;

    struct background {
        std::string identifier;
        proc::timeline_ptr timeline;
        std::optional<proc::sync_source> sync_source;
    };

    background _bg;

    timeline_exporter(std::string const &root_path, std::shared_ptr<task_queue> const &, task_priority const &,
                      proc::sample_rate_t const);

    void _prepare(timeline_exporter_ptr const &);
    void _receive_timeline_event(proc::timeline::event_t const &event, timeline_exporter_ptr const &exporter);
    void _receive_relayed_timeline_event(proc::timeline::relayed_event_t const &event,
                                         timeline_exporter_ptr const &exporter);
    void _receive_relayed_track_event(proc::track::relayed_event_t const &event, proc::track_index_t const trk_idx,
                                      timeline_exporter_ptr const &exporter);
    void _update_timeline(proc::timeline::track_map_t &&tracks, timeline_exporter_ptr const &exporter);
    void _insert_tracks(proc::timeline::inserted_event_t const &event, timeline_exporter_ptr const &exporter);
    void _erase_tracks(proc::timeline::erased_event_t const &event, timeline_exporter_ptr const &exporter);
    void _insert_modules(proc::track_index_t const trk_idx, proc::track::inserted_event_t const &event,
                         timeline_exporter_ptr const &exporter);
    void _erase_modules(proc::track_index_t const trk_idx, proc::track::erased_event_t const &event,
                        timeline_exporter_ptr const &exporter);
    void _insert_module(proc::track_index_t const trk_idx, proc::time::range const range,
                        proc::module_vector::inserted_event_t const &event, timeline_exporter_ptr const &exporter);
    void _erase_module(proc::track_index_t const trk_idx, proc::time::range const range,
                       proc::module_vector::erased_event_t const &event, timeline_exporter_ptr const &exporter);
    void _push_export_task(proc::time::range const &range, timeline_exporter_ptr const &exporter);
    void _export_fragments(proc::time::range const &frags_range, task const &task,
                           timeline_exporter_wptr const &weak_exporter);
    [[nodiscard]] std::optional<error> _export_fragment_on_bg(proc::time::range const &frag_range,
                                                              proc::stream const &stream);
    [[nodiscard]] std::optional<error> _remove_fragments_on_bg(proc::time::range const &frags_range, task const &task);
    void _send_method_on_bg(method const type, std::optional<proc::time::range> const &range,
                            timeline_exporter_wptr const &weak_exporter);
    void _send_error_on_bg(error const type, std::optional<proc::time::range> const &range,
                           timeline_exporter_wptr const &weak_exporter);
    void _send_event_on_bg(event event, timeline_exporter_wptr const &weak_exporter);
    proc::sync_source const &_sync_source_on_bg();
};
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::timeline_exporter::method const &);
std::string to_string(playing::timeline_exporter::error const &);
};  // namespace yas

std::ostream &operator<<(std::ostream &, yas::playing::timeline_exporter::method const &);
std::ostream &operator<<(std::ostream &, yas::playing::timeline_exporter::error const &);
