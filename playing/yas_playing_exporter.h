//
//  yas_playing_exporter.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <playing/yas_playing_exporter_resource.h>
#include <playing/yas_playing_timeline_container.h>

namespace yas::playing {
struct exporter final : exportable {
    using method_t = exporter_method;
    using error_t = exporter_error;
    using result_t = exporter_result_t;
    using event_t = exporter_event;
    using task_priority_t = exporter_task_priority;

    void set_timeline_container(timeline_container_ptr const &) override;

    chaining::chain_unsync_t<event_t> event_chain() const override;

    static exporter_ptr make_shared(std::string const &root_path, std::shared_ptr<task_queue> const &,
                                    task_priority_t const &);

   private:
    std::shared_ptr<task_queue> const _queue;
    task_priority_t const _priority;
    chaining::value::holder_ptr<timeline_container_ptr> const _container;
    exporter_resource_ptr const _resource;

    chaining::observer_pool _pool;

    exporter(std::string const &root_path, std::shared_ptr<task_queue> const &, task_priority_t const &);

    void _receive_timeline_event(proc::timeline::event_t const &event);
    void _receive_relayed_timeline_event(proc::timeline::relayed_event_t const &event);
    void _receive_relayed_track_event(proc::track::relayed_event_t const &event, track_index_t const trk_idx);
    void _update_timeline(proc::timeline::track_map_t &&tracks);
    void _insert_tracks(proc::timeline::inserted_event_t const &event);
    void _erase_tracks(proc::timeline::erased_event_t const &event);
    void _insert_modules(track_index_t const trk_idx, proc::track::inserted_event_t const &event);
    void _erase_modules(track_index_t const trk_idx, proc::track::erased_event_t const &event);
    void _insert_module(track_index_t const trk_idx, proc::time::range const range,
                        proc::module_vector::inserted_event_t const &event);
    void _erase_module(track_index_t const trk_idx, proc::time::range const range,
                       proc::module_vector::erased_event_t const &event);
    void _push_export_task(proc::time::range const &range);
};
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::exporter::method_t const &);
std::string to_string(playing::exporter::error_t const &);
};  // namespace yas

std::ostream &operator<<(std::ostream &, yas::playing::exporter::method_t const &);
std::ostream &operator<<(std::ostream &, yas::playing::exporter::error_t const &);
