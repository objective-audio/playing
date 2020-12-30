//
//  yas_playing_exporter_resource.cpp
//

#include "yas_playing_exporter_resource.h"

#include <cpp_utils/yas_file_manager.h>
#include <cpp_utils/yas_thread.h>
#include <dispatch/dispatch.h>
#include <processing/yas_processing_umbrella.h>

#include "yas_playing_numbers_file.h"
#include "yas_playing_path.h"
#include "yas_playing_signal_file.h"

using namespace yas;
using namespace yas::playing;

exporter_resource::exporter_resource(std::string const &root_path) : root_path(root_path) {
}

void exporter_resource::export_fragments_on_task(exporter_resource_ptr const &resource,
                                                 proc::time::range const &frags_range, task const &task) {
    assert(!thread::is_main());

    if (task.is_canceled()) {
        return;
    }

    this->timeline->process(frags_range, resource->sync_source.value(),
                            [&resource, &task, this](proc::time::range const &range, proc::stream const &stream) {
                                if (task.is_canceled()) {
                                    return proc::continuation::abort;
                                }

                                if (auto error = this->export_fragment_on_task(resource, range, stream)) {
                                    resource->send_error_on_task(*error, range);
                                } else {
                                    resource->send_method_on_task(exporter_method::export_ended, range);
                                }

                                return proc::continuation::keep;
                            });
}

[[nodiscard]] std::optional<exporter_error> exporter_resource::export_fragment_on_task(
    exporter_resource_ptr const &resource, proc::time::range const &frag_range, proc::stream const &stream) {
    assert(!thread::is_main());

    auto const &sync_source = resource->sync_source.value();
    path::timeline const tl_path{this->root_path, resource->identifier, sync_source.sample_rate};

    auto const frag_idx = frag_range.frame / stream.sync_source().sample_rate;

    for (auto const &ch_pair : stream.channels()) {
        auto const &ch_idx = ch_pair.first;
        auto const &channel = ch_pair.second;

        path::channel const ch_path{tl_path, ch_idx};
        auto const frag_path = path::fragment{ch_path, frag_idx};
        std::string const frag_path_str = frag_path.string();

        auto remove_result = file_manager::remove_content(frag_path_str);
        if (!remove_result) {
            return exporter_error::remove_fragment_failed;
        }

        if (channel.events().size() == 0) {
            return std::nullopt;
        }

        auto const create_result = file_manager::create_directory_if_not_exists(frag_path_str);
        if (!create_result) {
            return exporter_error::create_directory_failed;
        }

        for (auto const &event_pair : channel.filtered_events<proc::signal_event>()) {
            proc::time::range const &range = event_pair.first;
            proc::signal_event_ptr const &event = event_pair.second;

            auto const signal_path_str = path::signal_event{frag_path, range, event->sample_type()}.string();

            if (auto const result = signal_file::write(signal_path_str, *event); !result) {
                return exporter_error::write_signal_failed;
            }
        }

        if (auto const number_events = channel.filtered_events<proc::number_event>(); number_events.size() > 0) {
            auto const number_path_str = path::number_events{frag_path}.string();

            if (auto const result = numbers_file::write(number_path_str, number_events); !result) {
                return exporter_error::write_numbers_failed;
            }
        }
    }

    return std::nullopt;
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

exporter_resource_ptr exporter_resource::make_shared(std::string const &root_path) {
    return exporter_resource_ptr{new exporter_resource{root_path}};
}
