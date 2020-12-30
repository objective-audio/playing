//
//  yas_playing_exporter_resource.h
//

#pragma once

#include <playing/yas_playing_exporter_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct exporter_resource {
    std::string identifier;
    proc::timeline_ptr timeline;
    std::optional<proc::sync_source> sync_source;

    chaining::notifier_ptr<exporter_event> const event_notifier = chaining::notifier<exporter_event>::make_shared();

    void send_method_on_task(exporter_method const type, std::optional<proc::time::range> const &range);
    void send_error_on_task(exporter_error const type, std::optional<proc::time::range> const &range);

    static exporter_resource_ptr make_shared();

   private:
    exporter_resource();

    void _send_event_on_task(exporter_event event);
};
}  // namespace yas::playing
