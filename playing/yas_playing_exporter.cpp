//
//  yas_playing_exporter.cpp
//

#include "yas_playing_exporter.h"

#include <audio/yas_audio_file.h>
#include <audio/yas_audio_pcm_buffer.h>
#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_fast_each.h>
#include <cpp_utils/yas_file_manager.h>
#include <cpp_utils/yas_task.h>
#include <cpp_utils/yas_thread.h>
#include <processing/yas_processing_umbrella.h>

#include "yas_playing_math.h"
#include "yas_playing_numbers_file.h"
#include "yas_playing_path.h"
#include "yas_playing_signal_file.h"
#include "yas_playing_timeline_canceling.h"
#include "yas_playing_timeline_utils.h"
#include "yas_playing_types.h"

using namespace yas;
using namespace yas::playing;

exporter::exporter(std::string const &root_path, std::shared_ptr<task_queue> const &queue,
                   task_priority_t const &priority, proc::sample_rate_t const sample_rate)
    : _queue(queue),
      _priority(priority),
      _src_container(
          chaining::value::holder<timeline_container_ptr>::make_shared(timeline_container::make_shared(sample_rate))),
      _resource(exporter_resource::make_shared(root_path)) {
    this->_pool += this->_src_container->chain()
                       .perform([observer = chaining::any_observer_ptr{nullptr},
                                 this](timeline_container_ptr const &container) mutable {
                           if (observer) {
                               observer->invalidate();
                               observer = nullptr;
                           }

                           if (auto const &timeline = container->timeline()) {
                               observer = timeline->get()
                                              ->chain()
                                              .perform([this](proc::timeline::event_t const &event) {
                                                  this->_receive_timeline_event(event);
                                              })
                                              .sync();
                           }
                       })
                       .end();
}

void exporter::set_timeline_container(timeline_container_ptr const &container) {
    assert(thread::is_main());

    this->_src_container->set_value(container);
}

chaining::chain_unsync_t<exporter::event_t> exporter::event_chain() const {
    return this->_resource->event_notifier->chain();
}

void exporter::_receive_timeline_event(proc::timeline::event_t const &event) {
    switch (event.type()) {
        case proc::timeline::event_type_t::fetched: {
            auto const fetched_event = event.get<proc::timeline::fetched_event_t>();
            this->_update_timeline(proc::copy_tracks(fetched_event.elements));
        } break;
        case proc::timeline::event_type_t::inserted: {
            this->_insert_tracks(event.get<proc::timeline::inserted_event_t>());
        } break;
        case proc::timeline::event_type_t::erased: {
            this->_erase_tracks(event.get<proc::timeline::erased_event_t>());
        } break;
        case proc::timeline::event_type_t::relayed: {
            this->_receive_relayed_timeline_event(event.get<proc::timeline::relayed_event_t>());
        } break;
        default:
            throw std::runtime_error("unreachable code.");
    }
}

void exporter::_receive_relayed_timeline_event(proc::timeline::relayed_event_t const &event) {
    switch (event.relayed.type()) {
        case proc::track::event_type_t::inserted: {
            this->_insert_modules(event.key, event.relayed.get<proc::track::inserted_event_t>());
        } break;
        case proc::track::event_type_t::erased: {
            this->_erase_modules(event.key, event.relayed.get<proc::track::erased_event_t>());
        } break;
        case proc::track::event_type_t::relayed: {
            this->_receive_relayed_track_event(event.relayed.get<proc::track::relayed_event_t>(), event.key);
        } break;
        default:
            throw std::runtime_error("unreachable code.");
    }
}

void exporter::_receive_relayed_track_event(proc::track::relayed_event_t const &event,
                                            proc::track_index_t const trk_idx) {
    switch (event.relayed.type()) {
        case proc::module_vector::event_type_t::inserted:
            this->_insert_module(trk_idx, event.key, event.relayed.get<proc::module_vector::inserted_event_t>());
            break;
        case proc::module_vector::event_type_t::erased:
            this->_erase_module(trk_idx, event.key, event.relayed.get<proc::module_vector::erased_event_t>());
            break;
        default:
            throw std::runtime_error("unreachable code.");
    }
}

void exporter::_update_timeline(proc::timeline::track_map_t &&tracks) {
    assert(thread::is_main());

    this->_queue->cancel_all();

    auto const &container = this->_src_container->raw();

    auto task = task::make_shared(
        [resource = this->_resource, tracks = std::move(tracks), identifier = container->identifier(),
         sample_rate = container->sample_rate()](yas::task const &task) mutable {
            resource->identifier = identifier;
            resource->timeline = proc::timeline::make_shared(std::move(tracks));
            resource->sync_source.emplace(sample_rate, sample_rate);

            if (task.is_canceled()) {
                return;
            }

            resource->send_method_on_task(method_t::reset, std::nullopt);

            if (auto const result = file_manager::remove_content(resource->root_path); !result) {
                std::runtime_error("remove timeline root directory failed.");
            }

            if (task.is_canceled()) {
                return;
            }

            proc::timeline_ptr const &timeline = resource->timeline;

            auto total_range = timeline->total_range();
            if (!total_range.has_value()) {
                return;
            }

            auto const &sync_source = resource->sync_source.value();
            auto const frags_range = timeline_utils::fragments_range(*total_range, sync_source.sample_rate);

            resource->send_method_on_task(method_t::export_began, frags_range);

            resource->export_fragments_on_task(resource, frags_range, task);
        },
        {.priority = this->_priority.timeline});

    this->_queue->push_back(std::move(task));
}

void exporter::_insert_tracks(proc::timeline::inserted_event_t const &event) {
    assert(thread::is_main());

    auto tracks = proc::copy_tracks(event.elements);

    std::optional<proc::time::range> total_range = proc::total_range(tracks);

    for (auto &pair : tracks) {
        auto insert_task = task::make_shared(
            [resource = this->_resource, trk_idx = pair.first, track = std::move(pair.second)](auto const &) mutable {
                resource->timeline->insert_track(trk_idx, std::move(track));
            },
            {.priority = this->_priority.timeline});

        this->_queue->push_back(insert_task);
    }

    if (total_range) {
        this->_push_export_task(*total_range);
    }
}

void exporter::_erase_tracks(proc::timeline::erased_event_t const &event) {
    assert(thread::is_main());

    auto track_indices = to_vector<proc::track_index_t>(event.elements, [](auto const &pair) { return pair.first; });

    std::optional<proc::time::range> total_range = proc::total_range(event.elements);

    for (auto &trk_idx : track_indices) {
        auto erase_task = task::make_shared(
            [resource = this->_resource, trk_idx = trk_idx](auto const &) { resource->timeline->erase_track(trk_idx); },
            {.priority = this->_priority.timeline});

        this->_queue->push_back(std::move(erase_task));
    }

    if (total_range) {
        this->_push_export_task(*total_range);
    }
}

void exporter::_insert_modules(proc::track_index_t const trk_idx, proc::track::inserted_event_t const &event) {
    assert(thread::is_main());

    auto modules = proc::copy_to_modules(event.elements);

    for (auto &pair : modules) {
        auto const &range = pair.first;
        auto task = task::make_shared(
            [resource = this->_resource, trk_idx, range = range, modules = std::move(pair.second)](auto const &) {
                auto const &track = resource->timeline->track(trk_idx);
                assert(track->modules().count(range) == 0);
                for (auto &module : modules) {
                    track->push_back_module(std::move(module), range);
                }
            },
            {.priority = this->_priority.timeline});

        this->_queue->push_back(std::move(task));
    }

    for (auto const &pair : modules) {
        auto const &range = pair.first;
        this->_push_export_task(range);
    }
}

void exporter::_erase_modules(proc::track_index_t const trk_idx, proc::track::erased_event_t const &event) {
    assert(thread::is_main());

    auto modules = proc::copy_to_modules(event.elements);

    for (auto &pair : modules) {
        auto const &range = pair.first;
        auto task = task::make_shared(
            [resource = this->_resource, trk_idx, range = range, module = std::move(pair.second)](auto const &) {
                auto const &track = resource->timeline->track(trk_idx);
                assert(track->modules().count(range) > 0);
                track->erase_modules_for_range(range);
            },
            {.priority = this->_priority.timeline});

        this->_queue->push_back(std::move(task));
    }

    for (auto const &pair : modules) {
        auto const &range = pair.first;
        this->_push_export_task(range);
    }
}

void exporter::_insert_module(proc::track_index_t const trk_idx, proc::time::range const range,
                              proc::module_vector::inserted_event_t const &event) {
    assert(thread::is_main());

    auto task = task::make_shared(
        [resource = this->_resource, trk_idx, range, module_idx = event.index,
         module = event.element->copy()](auto const &) {
            auto const &track = resource->timeline->track(trk_idx);
            assert(track->modules().count(range) > 0);
            track->insert_module(std::move(module), module_idx, range);
        },
        {.priority = this->_priority.timeline});

    this->_queue->push_back(std::move(task));

    this->_push_export_task(range);
}

void exporter::_erase_module(proc::track_index_t const trk_idx, proc::time::range const range,
                             proc::module_vector::erased_event_t const &event) {
    assert(thread::is_main());

    auto task = task::make_shared(
        [resource = this->_resource, trk_idx, range, module_idx = event.index](auto const &) {
            auto const &track = resource->timeline->track(trk_idx);
            assert(track->modules().count(range) > 0);
            track->erase_module_at(module_idx, range);
        },
        {.priority = this->_priority.timeline});

    this->_queue->push_back(std::move(task));

    this->_push_export_task(range);
}

void exporter::_push_export_task(proc::time::range const &range) {
    this->_queue->cancel_for_id(timeline_range_cancel_request::make_shared(range));

    auto export_task = task::make_shared(
        [resource = this->_resource, range](task const &task) {
            auto const &sync_source = resource->sync_source.value();
            auto frags_range = timeline_utils::fragments_range(range, sync_source.sample_rate);

            resource->send_method_on_task(method_t::export_began, frags_range);

            if (auto const error = resource->remove_fragments_on_task(resource, frags_range, task)) {
                resource->send_error_on_task(*error, range);
                return;
            } else {
                resource->export_fragments_on_task(resource, frags_range, task);
            }
        },
        {.priority = this->_priority.fragment, .cancel_id = timeline_cancel_matcher::make_shared(range)});

    this->_queue->push_back(std::move(export_task));
}

exporter_ptr exporter::make_shared(std::string const &root_path, std::shared_ptr<task_queue> const &task_queue,
                                   task_priority_t const &task_priority, proc::sample_rate_t const sample_rate) {
    return exporter_ptr(new exporter{root_path, task_queue, task_priority, sample_rate});
}

std::string yas::to_string(exporter::method_t const &method) {
    switch (method) {
        case exporter::method_t::reset:
            return "reset";
        case exporter::method_t::export_began:
            return "export_began";
        case exporter::method_t::export_ended:
            return "export_ended";
    }
}

std::string yas::to_string(exporter::error_t const &error) {
    switch (error) {
        case exporter::error_t::remove_fragment_failed:
            return "remove_fragment_failed";
        case exporter::error_t::create_directory_failed:
            return "create_directory_failed";
        case exporter::error_t::write_signal_failed:
            return "write_signal_failed";
        case exporter::error_t::write_numbers_failed:
            return "write_numbers_failed";
        case exporter::error_t::get_content_paths_failed:
            return "get_content_paths_failed";
    }
}

std::ostream &operator<<(std::ostream &stream, exporter::method_t const &value) {
    stream << to_string(value);
    return stream;
}

std::ostream &operator<<(std::ostream &stream, exporter::error_t const &value) {
    stream << to_string(value);
    return stream;
}