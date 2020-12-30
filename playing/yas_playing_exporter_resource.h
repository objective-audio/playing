//
//  yas_playing_exporter_resource.h
//

#pragma once

#include <playing/yas_playing_ptr.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct exporter_resource {
    std::string identifier;
    proc::timeline_ptr timeline;
    std::optional<proc::sync_source> sync_source;

    static exporter_resource_ptr make_shared();

   private:
    exporter_resource();
};
}  // namespace yas::playing
