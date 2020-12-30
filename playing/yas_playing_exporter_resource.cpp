//
//  yas_playing_exporter_resource.cpp
//

#include "yas_playing_exporter_resource.h"

#include <dispatch/dispatch.h>

using namespace yas;
using namespace yas::playing;

exporter_resource::exporter_resource() {
}

exporter_resource_ptr exporter_resource::make_shared() {
    return exporter_resource_ptr{new exporter_resource{}};
}

void exporter_resource::send_event_on_task(exporter_event event) {
    auto lambda = [event = std::move(event), weak_notifier = to_weak(this->event_notifier)] {
        if (auto notifier = weak_notifier.lock()) {
            notifier->notify(event);
        }
    };

    dispatch_async(dispatch_get_main_queue(), ^{
        lambda();
    });
}
