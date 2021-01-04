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

    void export_timeline_on_task(proc::timeline::track_map_t &&, std::string const &identifier, sample_rate_t const &,
                                 yas::task const &);
    void insert_track_on_task(proc::track_index_t const, proc::track_ptr &&track);
    void erase_track_on_task(proc::track_index_t const);
    void insert_modules_on_task(proc::track_index_t const, proc::time::range const &, std::vector<proc::module_ptr> &&);
    void erase_modules_on_task(proc::track_index_t const, proc::time::range const &);
    void insert_module(proc::module_ptr const &, std::size_t const module_idx, proc::track_index_t const,
                       proc::time::range const);
    void erase_module(std::size_t const module_idx, proc::track_index_t const, proc::time::range const);

    void export_on_task(proc::time::range const &, task const &);

    [[nodiscard]] static exporter_resource_ptr make_shared(std::string const &root_path);

   private:
    exporter_resource(std::string const &root_path);

    void _send_method_on_task(exporter_method const type, std::optional<proc::time::range> const &range);
    void _send_error_on_task(exporter_error const type, std::optional<proc::time::range> const &range);
    void _send_event_on_task(exporter_event event);

    void _export_fragments_on_task(proc::time::range const &, task const &);
    [[nodiscard]] std::optional<exporter_error> _export_fragment_on_task(proc::time::range const &frag_range,
                                                                         proc::stream const &stream);
    [[nodiscard]] std::optional<exporter_error> _remove_fragments_on_task(proc::time::range const &frags_range,
                                                                          task const &task);
};
}  // namespace yas::playing
