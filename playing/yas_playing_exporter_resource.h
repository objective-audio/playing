//
//  yas_playing_exporter_resource.h
//

#pragma once

#include <playing/yas_playing_exporter_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct exporter_resource {
    std::string const root_path;
    std::string identifier;
    proc::timeline_ptr timeline;
    std::optional<proc::sync_source> sync_source;

    chaining::notifier_ptr<exporter_event> const event_notifier = chaining::notifier<exporter_event>::make_shared();

    void export_timeline_on_task(proc::timeline::track_map_t &&, std::string const &identifier,
                                 sample_rate_t const &sample_rate, yas::task const &task);
    void insert_track_on_task(proc::track_index_t const trk_idx, proc::track_ptr &&track);
    void erase_track_on_task(proc::track_index_t const trk_idx);
    void insert_modules_on_task(proc::track_index_t const trk_idx, proc::time::range const &range,
                                std::vector<proc::module_ptr> &&modules);
    void erase_modules_on_task(proc::track_index_t const trk_idx, proc::time::range const &range);
    void insert_module(proc::module_ptr const &module, std::size_t const module_idx, proc::track_index_t const trk_idx,
                       proc::time::range const range);
    void push_export_on_task(proc::time::range const &, task const &task);

    void export_fragments_on_task(proc::time::range const &frags_range, task const &task);
    [[nodiscard]] std::optional<exporter_error> export_fragment_on_task(proc::time::range const &frag_range,
                                                                        proc::stream const &stream);
    [[nodiscard]] std::optional<exporter_error> remove_fragments_on_task(proc::time::range const &frags_range,
                                                                         task const &task);

    void send_method_on_task(exporter_method const type, std::optional<proc::time::range> const &range);
    void send_error_on_task(exporter_error const type, std::optional<proc::time::range> const &range);

    [[nodiscard]] static exporter_resource_ptr make_shared(std::string const &root_path);

   private:
    exporter_resource(std::string const &root_path);

    void _send_event_on_task(exporter_event event);
};
}  // namespace yas::playing
