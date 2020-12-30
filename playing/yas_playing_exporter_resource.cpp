//
//  yas_playing_exporter_resource.cpp
//

#include "yas_playing_exporter_resource.h"

#include <cpp_utils/yas_thread.h>
#include <dispatch/dispatch.h>

using namespace yas;
using namespace yas::playing;

exporter_resource::exporter_resource() {
}

exporter_resource_ptr exporter_resource::make_shared() {
    return exporter_resource_ptr{new exporter_resource{}};
}

void exporter_resource::send_method_on_task(exporter_method const type, std::optional<proc::time::range> const &range) {
    assert(!thread::is_main());

    this->_send_event_on_task(exporter_event{.result = exporter_result_t{type}, .range = range});
}

void exporter_resource::send_error_on_task(exporter_error const type, std::optional<proc::time::range> const &range) {
    assert(!thread::is_main());

    this->_send_event_on_task(exporter_event{.result = exporter_result_t{type}, .range = range});
}

void exporter_resource::_send_event_on_task(exporter_event event) {
    auto lambda = [event = std::move(event), weak_notifier = to_weak(this->event_notifier)] {
        if (auto notifier = weak_notifier.lock()) {
            notifier->notify(event);
        }
    };

    dispatch_async(dispatch_get_main_queue(), ^{
        lambda();
    });
}
