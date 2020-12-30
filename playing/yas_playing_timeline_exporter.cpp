//
//  yas_playing_timeline_exporter.cpp
//

#include "yas_playing_timeline_exporter.h"

#include <audio/yas_audio_file.h>
#include <audio/yas_audio_pcm_buffer.h>
#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_fast_each.h>
#include <cpp_utils/yas_file_manager.h>
#include <cpp_utils/yas_task.h>
#include <cpp_utils/yas_thread.h>
#include <cpp_utils/yas_to_integer.h>
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

timeline_exporter::timeline_exporter(std::string const &root_path, std::shared_ptr<task_queue> const &queue,
                                     task_priority const &priority, proc::sample_rate_t const sample_rate)
    : _root_path(root_path),
      _queue(queue),
      _priority(priority),
      _src_container(
          chaining::value::holder<timeline_container_ptr>::make_shared(timeline_container::make_shared(sample_rate))) {
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

void timeline_exporter::set_timeline_container(timeline_container_ptr const &container) {
    assert(thread::is_main());

    this->_src_container->set_value(container);
}

chaining::chain_unsync_t<timeline_exporter::event> timeline_exporter::event_chain() const {
    return this->_event_notifier->chain();
}

void timeline_exporter::_receive_timeline_event(proc::timeline::event_t const &event) {
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

void timeline_exporter::_receive_relayed_timeline_event(proc::timeline::relayed_event_t const &event) {
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

void timeline_exporter::_receive_relayed_track_event(proc::track::relayed_event_t const &event,
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

void timeline_exporter::_update_timeline(proc::timeline::track_map_t &&tracks) {
    assert(thread::is_main());

    this->_queue->cancel_all();

    auto const &container = this->_src_container->raw();

    auto task = task::make_shared(
        [tracks = std::move(tracks), identifier = container->identifier(), sample_rate = container->sample_rate(),
         weak_exporter = this->_weak_exporter](yas::task const &task) mutable {
            if (auto exporter = weak_exporter.lock()) {
                auto &bg = exporter->_bg;
                bg.identifier = identifier;
                bg.timeline = proc::timeline::make_shared(std::move(tracks));
                bg.sync_source.emplace(sample_rate, sample_rate);

                if (task.is_canceled()) {
                    return;
                }

                exporter->_send_method_on_bg(method::reset, std::nullopt);

                auto const &root_path = exporter->_root_path;

                if (auto const result = file_manager::remove_content(root_path); !result) {
                    std::runtime_error("remove timeline root directory failed.");
                }

                if (task.is_canceled()) {
                    return;
                }

                proc::timeline_ptr const &timeline = exporter->_bg.timeline;

                auto total_range = timeline->total_range();
                if (!total_range.has_value()) {
                    return;
                }

                auto const &sync_source = exporter->_sync_source_on_bg();
                auto const frags_range = timeline_utils::fragments_range(*total_range, sync_source.sample_rate);

                exporter->_send_method_on_bg(method::export_began, frags_range);

                exporter->_export_fragments(frags_range, task);
            }
        },
        {.priority = this->_priority.timeline});

    this->_queue->push_back(std::move(task));
}

void timeline_exporter::_insert_tracks(proc::timeline::inserted_event_t const &event) {
    assert(thread::is_main());

    auto tracks = proc::copy_tracks(event.elements);

    std::optional<proc::time::range> total_range = proc::total_range(tracks);

    for (auto &pair : tracks) {
        auto insert_task = task::make_shared(
            [trk_idx = pair.first, track = std::move(pair.second),
             weak_exporter = this->_weak_exporter](auto const &) mutable {
                if (auto exporter = weak_exporter.lock()) {
                    exporter->_bg.timeline->insert_track(trk_idx, std::move(track));
                }
            },
            {.priority = this->_priority.timeline});

        this->_queue->push_back(insert_task);
    }

    if (total_range) {
        this->_push_export_task(*total_range);
    }
}

void timeline_exporter::_erase_tracks(proc::timeline::erased_event_t const &event) {
    assert(thread::is_main());

    auto track_indices = to_vector<proc::track_index_t>(event.elements, [](auto const &pair) { return pair.first; });

    std::optional<proc::time::range> total_range = proc::total_range(event.elements);

    for (auto &trk_idx : track_indices) {
        auto erase_task = task::make_shared(
            [trk_idx = trk_idx, weak_exporter = this->_weak_exporter](auto const &) mutable {
                if (auto exporter = weak_exporter.lock()) {
                    exporter->_bg.timeline->erase_track(trk_idx);
                }
            },
            {.priority = this->_priority.timeline});

        this->_queue->push_back(std::move(erase_task));
    }

    if (total_range) {
        this->_push_export_task(*total_range);
    }
}

void timeline_exporter::_insert_modules(proc::track_index_t const trk_idx, proc::track::inserted_event_t const &event) {
    assert(thread::is_main());

    auto modules = proc::copy_to_modules(event.elements);

    for (auto &pair : modules) {
        auto const &range = pair.first;
        auto task = task::make_shared(
            [trk_idx, range = range, modules = std::move(pair.second),
             weak_exporter = this->_weak_exporter](auto const &) mutable {
                if (auto exporter = weak_exporter.lock()) {
                    auto const &track = exporter->_bg.timeline->track(trk_idx);
                    assert(track->modules().count(range) == 0);
                    for (auto &module : modules) {
                        track->push_back_module(std::move(module), range);
                    }
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

void timeline_exporter::_erase_modules(proc::track_index_t const trk_idx, proc::track::erased_event_t const &event) {
    assert(thread::is_main());

    auto modules = proc::copy_to_modules(event.elements);

    for (auto &pair : modules) {
        auto const &range = pair.first;
        auto task = task::make_shared(
            [trk_idx, range = range, module = std::move(pair.second),
             weak_exporter = this->_weak_exporter](auto const &) mutable {
                if (auto exporter = weak_exporter.lock()) {
                    auto const &track = exporter->_bg.timeline->track(trk_idx);
                    assert(track->modules().count(range) > 0);
                    track->erase_modules_for_range(range);
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

void timeline_exporter::_insert_module(proc::track_index_t const trk_idx, proc::time::range const range,
                                       proc::module_vector::inserted_event_t const &event) {
    assert(thread::is_main());

    auto task = task::make_shared(
        [trk_idx, range, module_idx = event.index, module = event.element->copy(),
         weak_exporter = this->_weak_exporter](auto const &) mutable {
            if (auto exporter = weak_exporter.lock()) {
                auto const &track = exporter->_bg.timeline->track(trk_idx);
                assert(track->modules().count(range) > 0);
                track->insert_module(std::move(module), module_idx, range);
            }
        },
        {.priority = this->_priority.timeline});

    this->_queue->push_back(std::move(task));

    this->_push_export_task(range);
}

void timeline_exporter::_erase_module(proc::track_index_t const trk_idx, proc::time::range const range,
                                      proc::module_vector::erased_event_t const &event) {
    assert(thread::is_main());

    auto task = task::make_shared(
        [trk_idx, range, module_idx = event.index, weak_exporter = this->_weak_exporter](auto const &) mutable {
            if (auto exporter = weak_exporter.lock()) {
                auto const &track = exporter->_bg.timeline->track(trk_idx);
                assert(track->modules().count(range) > 0);
                track->erase_module_at(module_idx, range);
            }
        },
        {.priority = this->_priority.timeline});

    this->_queue->push_back(std::move(task));

    this->_push_export_task(range);
}

void timeline_exporter::_push_export_task(proc::time::range const &range) {
    this->_queue->cancel_for_id(timeline_range_cancel_request::make_shared(range));

    auto export_task = task::make_shared(
        [range, weak_exporter = this->_weak_exporter](task const &task) {
            if (auto exporter = weak_exporter.lock()) {
                auto const &sync_source = exporter->_sync_source_on_bg();
                auto frags_range = timeline_utils::fragments_range(range, sync_source.sample_rate);

                exporter->_send_method_on_bg(method::export_began, frags_range);

                if (auto const error = exporter->_remove_fragments_on_bg(frags_range, task)) {
                    exporter->_send_error_on_bg(*error, range);
                    return;
                } else {
                    exporter->_export_fragments(frags_range, task);
                }
            }
        },
        {.priority = this->_priority.fragment, .cancel_id = timeline_cancel_matcher::make_shared(range)});

    this->_queue->push_back(std::move(export_task));
}

void timeline_exporter::_export_fragments(proc::time::range const &frags_range, task const &task) {
    assert(!thread::is_main());

    if (task.is_canceled()) {
        return;
    }

    this->_bg.timeline->process(frags_range, this->_sync_source_on_bg(),
                                [&task, this](proc::time::range const &range, proc::stream const &stream) {
                                    if (task.is_canceled()) {
                                        return proc::continuation::abort;
                                    }

                                    if (auto error = this->_export_fragment_on_bg(range, stream)) {
                                        this->_send_error_on_bg(*error, range);
                                    } else {
                                        this->_send_method_on_bg(method::export_ended, range);
                                    }

                                    return proc::continuation::keep;
                                });
}

[[nodiscard]] std::optional<timeline_exporter::error> timeline_exporter::_export_fragment_on_bg(
    proc::time::range const &frag_range, proc::stream const &stream) {
    assert(!thread::is_main());

    auto const &sync_source = this->_sync_source_on_bg();
    path::timeline const tl_path{this->_root_path, "0", sync_source.sample_rate};

    auto const frag_idx = frag_range.frame / stream.sync_source().sample_rate;

    for (auto const &ch_pair : stream.channels()) {
        auto const &ch_idx = ch_pair.first;
        auto const &channel = ch_pair.second;

        path::channel const ch_path{tl_path, ch_idx};
        auto const frag_path = path::fragment{ch_path, frag_idx};
        std::string const frag_path_str = frag_path.string();

        auto remove_result = file_manager::remove_content(frag_path_str);
        if (!remove_result) {
            return error::remove_fragment_failed;
        }

        if (channel.events().size() == 0) {
            return std::nullopt;
        }

        auto const create_result = file_manager::create_directory_if_not_exists(frag_path_str);
        if (!create_result) {
            return error::create_directory_failed;
        }

        for (auto const &event_pair : channel.filtered_events<proc::signal_event>()) {
            proc::time::range const &range = event_pair.first;
            proc::signal_event_ptr const &event = event_pair.second;

            auto const signal_path_str = path::signal_event{frag_path, range, event->sample_type()}.string();

            if (auto const result = signal_file::write(signal_path_str, *event); !result) {
                return error::write_signal_failed;
            }
        }

        if (auto const number_events = channel.filtered_events<proc::number_event>(); number_events.size() > 0) {
            auto const number_path_str = path::number_events{frag_path}.string();

            if (auto const result = numbers_file::write(number_path_str, number_events); !result) {
                return error::write_numbers_failed;
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<timeline_exporter::error> timeline_exporter::_remove_fragments_on_bg(
    proc::time::range const &frags_range, task const &task) {
    assert(!thread::is_main());

    auto const &root_path = this->_root_path;
    auto const &sync_source = this->_sync_source_on_bg();
    auto const &sample_rate = sync_source.sample_rate;
    path::timeline const tl_path{root_path, "0", sample_rate};

    auto ch_paths_result = file_manager::content_paths_in_directory(tl_path.string());
    if (!ch_paths_result) {
        if (ch_paths_result.error() == file_manager::content_paths_error::directory_not_found) {
            return std::nullopt;
        } else {
            return error::get_content_paths_failed;
        }
    }

    auto const ch_names = to_vector<std::string>(ch_paths_result.value(),
                                                 [](auto const &path) { return url{path}.last_path_component(); });

    auto const begin_frag_idx = frags_range.frame / sample_rate;
    auto const end_frag_idx = frags_range.next_frame() / sample_rate;

    for (auto const &ch_name : ch_names) {
        if (task.is_canceled()) {
            return std::nullopt;
        }

        auto const ch_idx = yas::to_integer<channel_index_t>(ch_name);
        path::channel const ch_path{tl_path, ch_idx};

        auto each = make_fast_each(begin_frag_idx, end_frag_idx);
        while (yas_each_next(each)) {
            auto const &frag_idx = yas_each_index(each);
            auto const frag_path_str = path::fragment{ch_path, frag_idx}.string();
            auto const remove_result = file_manager::remove_content(frag_path_str);
            if (!remove_result) {
                return error::remove_fragment_failed;
            }
        }
    }

    return std::nullopt;
}

void timeline_exporter::_send_method_on_bg(method const type, std::optional<proc::time::range> const &range) {
    assert(!thread::is_main());

    this->_send_event_on_bg(event{.result = result_t{type}, .range = range});
}

void timeline_exporter::_send_error_on_bg(error const type, std::optional<proc::time::range> const &range) {
    assert(!thread::is_main());

    this->_send_event_on_bg(event{.result = result_t{type}, .range = range});
}

void timeline_exporter::_send_event_on_bg(event event) {
    auto lambda = [this, event = std::move(event), weak_exporter = this->_weak_exporter] {
        if (auto exporter = weak_exporter.lock()) {
            exporter->_event_notifier->notify(event);
        }
    };

    dispatch_async(dispatch_get_main_queue(), ^{
        lambda();
    });
}

proc::sync_source const &timeline_exporter::_sync_source_on_bg() {
    return *this->_bg.sync_source;
}

timeline_exporter_ptr timeline_exporter::make_shared(std::string const &root_path,
                                                     std::shared_ptr<task_queue> const &task_queue,
                                                     task_priority const &task_priority,
                                                     proc::sample_rate_t const sample_rate) {
    auto shared = timeline_exporter_ptr(new timeline_exporter{root_path, task_queue, task_priority, sample_rate});
    shared->_weak_exporter = to_weak(shared);
    return shared;
}

std::string yas::to_string(timeline_exporter::method const &method) {
    switch (method) {
        case timeline_exporter::method::reset:
            return "reset";
        case timeline_exporter::method::export_began:
            return "export_began";
        case timeline_exporter::method::export_ended:
            return "export_ended";
    }
}

std::string yas::to_string(timeline_exporter::error const &error) {
    switch (error) {
        case timeline_exporter::error::remove_fragment_failed:
            return "remove_fragment_failed";
        case timeline_exporter::error::create_directory_failed:
            return "create_directory_failed";
        case timeline_exporter::error::write_signal_failed:
            return "write_signal_failed";
        case timeline_exporter::error::write_numbers_failed:
            return "write_numbers_failed";
        case timeline_exporter::error::get_content_paths_failed:
            return "get_content_paths_failed";
    }
}

std::ostream &operator<<(std::ostream &stream, timeline_exporter::method const &value) {
    stream << to_string(value);
    return stream;
}

std::ostream &operator<<(std::ostream &stream, timeline_exporter::error const &value) {
    stream << to_string(value);
    return stream;
}
