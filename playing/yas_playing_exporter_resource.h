//
//  yas_playing_exporter_resource.h
//

#pragma once

#include <playing/yas_playing_exporter_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct exporter_resource {
    std::string const root_path;
    std::string identifier;
    proc::timeline_ptr timeline;
    std::optional<proc::sync_source> sync_source;

    chaining::notifier_ptr<exporter_event> const event_notifier = chaining::notifier<exporter_event>::make_shared();

    void export_fragments_on_task(exporter_resource_ptr const &resource, proc::time::range const &frags_range,
                                  task const &task);
    [[nodiscard]] std::optional<exporter_error> export_fragment_on_task(exporter_resource_ptr const &resource,
                                                                        proc::time::range const &frag_range,
                                                                        proc::stream const &stream);

    void send_method_on_task(exporter_method const type, std::optional<proc::time::range> const &range);
    void send_error_on_task(exporter_error const type, std::optional<proc::time::range> const &range);

    static exporter_resource_ptr make_shared(std::string const &root_path);

   private:
    exporter_resource(std::string const &root_path);

    void _send_event_on_task(exporter_event event);
};
}  // namespace yas::playing
