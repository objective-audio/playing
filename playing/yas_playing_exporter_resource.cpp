//
//  yas_playing_exporter_resource.cpp
//

#include "yas_playing_exporter_resource.h"

using namespace yas;
using namespace yas::playing;

exporter_resource::exporter_resource() {
}

exporter_resource_ptr exporter_resource::make_shared() {
    return exporter_resource_ptr{new exporter_resource{}};
}
