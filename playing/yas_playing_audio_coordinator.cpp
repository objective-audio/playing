//
//  yas_playing_audio_coordinator.cpp
//

#include "yas_playing_audio_coordinator.h"

#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_fast_each.h>

#include <thread>

#include "yas_playing_audio_buffering.h"
#include "yas_playing_audio_buffering_channel.h"
#include "yas_playing_audio_buffering_element.h"
#include "yas_playing_audio_player_resource.h"
#include "yas_playing_audio_reading.h"
#include "yas_playing_audio_utils.h"
#include "yas_playing_timeline_utils.h"
#include "yas_playing_types.h"

using namespace yas;
using namespace yas::playing;

audio_coordinator::audio_coordinator(std::string const &root_path, audio::io_device_ptr const &device)
    : _root_path(root_path),
      _device(device),
      _player(audio_player::make_shared(
          this->_renderer, this->_root_path, this->_worker, {.setup = 100, .rendering = 101},
          audio_player_resource::make_shared(audio_reading::make_shared(),
                                             audio_buffering::make_shared(3, root_path, audio_utils::make_channel)))) {
    this->_worker->start();
}

void audio_coordinator::set_playing(bool const is_playing) {
    this->_player->set_playing(is_playing);
}

void audio_coordinator::seek(frame_index_t const frame) {
    this->_player->seek(frame);
}

void audio_coordinator::overwrite(proc::time::range const &range) {
    auto &player = this->_player;
    auto const sample_rate = static_cast<proc::sample_rate_t>(this->sample_rate());
    proc::time::range const frags_range = timeline_utils::fragments_range(range, sample_rate);
    auto const begin_frag_idx = frags_range.frame / sample_rate;
    auto const next_frag_idx = frags_range.next_frame() / sample_rate;
    auto frag_each = make_fast_each(begin_frag_idx, next_frag_idx);
    auto const ch_count = this->channel_count();
    while (yas_each_next(frag_each)) {
        auto ch_each = make_fast_each(ch_count);
        while (yas_each_next(ch_each)) {
            player->overwrite(yas_each_index(ch_each), yas_each_index(frag_each));
        }
    }
}

bool audio_coordinator::is_playing() const {
    return this->_player->is_playing();
}

frame_index_t audio_coordinator::current_frame() const {
    return this->_player->current_frame();
}

sample_rate_t audio_coordinator::sample_rate() const {
    return this->_renderer->sample_rate();
}

audio::pcm_format audio_coordinator::pcm_format() const {
    return this->_renderer->pcm_format();
}

std::size_t audio_coordinator::channel_count() const {
    return this->_renderer->channel_count();
}

chaining::chain_sync_t<audio_configuration> audio_coordinator::configuration_chain() const {
    return this->_renderer->configuration_chain();
}

chaining::chain_sync_t<bool> audio_coordinator::is_playing_chain() const {
    return this->_player->is_playing_chain();
}

audio_coordinator_ptr audio_coordinator::make_shared(std::string const &root_path, audio::io_device_ptr const &device) {
    return audio_coordinator_ptr(new audio_coordinator{root_path, device});
}
