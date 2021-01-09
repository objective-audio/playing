//
//  yas_playing_coordinator.cpp
//

#include "yas_playing_coordinator.h"

#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_fast_each.h>

#include <thread>

#include "yas_playing_buffering_channel.h"
#include "yas_playing_buffering_element.h"
#include "yas_playing_buffering_resource.h"
#include "yas_playing_channel_mapping.h"
#include "yas_playing_exporter.h"
#include "yas_playing_player_resource.h"
#include "yas_playing_reading_resource.h"
#include "yas_playing_timeline_utils.h"
#include "yas_playing_types.h"

using namespace yas;
using namespace yas::playing;

coordinator::coordinator(std::string const &root_path, std::string const &identifier,
                         audio::io_device_ptr const &device)
    : _identifier(identifier),
      _device(device),
      _player(player::make_shared(
          root_path, identifier, this->_renderer, this->_worker, {},
          player_resource::make_shared(
              reading_resource::make_shared(),
              buffering_resource::make_shared(3, root_path, identifier, playing::make_buffering_channel)))),
      _exporter(exporter::make_shared(root_path, std::make_shared<task_queue>(2), {.timeline = 0, .fragment = 1})) {
    this->_exporter->event_chain()
        .perform([this](exporter_event const &event) {
            if (event.result.is_success()) {
                if (event.result.value() == exporter_method::export_ended) {
                    if (event.range.has_value()) {
                        this->overwrite(event.range.value());
                    }
                }
            }
        })
        .end()
        ->add_to(this->_pool);

    this->_renderer->configuration_chain()
        .perform([this](auto const &) { this->_update_exporter(); })
        .end()
        ->add_to(this->_pool);

    this->_worker->start();
}

void coordinator::set_timeline(proc::timeline_ptr const &timeline) {
    this->_timeline = timeline;
    this->_update_exporter();
}

void coordinator::set_channel_mapping(channel_mapping_ptr const &ch_mapping) {
    this->_player->set_channel_mapping(ch_mapping);
}

void coordinator::set_playing(bool const is_playing) {
    this->_player->set_playing(is_playing);
}

void coordinator::seek(frame_index_t const frame) {
    this->_player->seek(frame);
}

void coordinator::overwrite(proc::time::range const &range) {
    auto &player = this->_player;
    auto const sample_rate = this->sample_rate();

    proc::time::range const frags_range = timeline_utils::fragments_range(range, sample_rate);

    auto const begin_frag_idx = frags_range.frame / sample_rate;
    auto const next_frag_idx = frags_range.next_frame() / sample_rate;
    auto frag_each = make_fast_each(begin_frag_idx, next_frag_idx);

    while (yas_each_next(frag_each)) {
        player->overwrite(std::nullopt, yas_each_index(frag_each));
    }
}

bool coordinator::is_playing() const {
    return this->_player->is_playing();
}

frame_index_t coordinator::current_frame() const {
    return this->_player->current_frame();
}

sample_rate_t coordinator::sample_rate() const {
    return this->_renderer->sample_rate();
}

audio::pcm_format coordinator::pcm_format() const {
    return this->_renderer->pcm_format();
}

std::size_t coordinator::channel_count() const {
    return this->_renderer->channel_count();
}

chaining::chain_sync_t<configuration> coordinator::configuration_chain() const {
    return this->_renderer->configuration_chain();
}

chaining::chain_sync_t<bool> coordinator::is_playing_chain() const {
    return this->_player->is_playing_chain();
}

void coordinator::_update_exporter() {
    auto const container =
        timeline_container::make_shared(this->_identifier, this->_renderer->sample_rate(), this->_timeline);
    this->_exporter->set_timeline_container(container);
}

coordinator_ptr coordinator::make_shared(std::string const &root_path, std::string const &identifier,
                                         audio::io_device_ptr const &device) {
    return coordinator_ptr(new coordinator{root_path, identifier, device});
}
