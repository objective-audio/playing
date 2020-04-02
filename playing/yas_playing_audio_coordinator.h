//
//  yas_playing_audio_coordinator.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <chaining/yas_chaining_umbrella.h>
#include <processing/yas_processing_time.h>

#include "yas_playing_audio_configulation.h"
#include "yas_playing_audio_player.h"
#include "yas_playing_audio_renderer.h"
#include "yas_playing_loading_state.h"
#include "yas_playing_ptr.h"
#include "yas_playing_types.h"

namespace yas::playing {
struct audio_coordinator {
    void set_playing(bool const);
    void seek(frame_index_t const);
    void reload(proc::time::range const &);

    bool is_playing() const;
    frame_index_t play_frame() const;

    [[nodiscard]] proc::sample_rate_t sample_rate() const;
    [[nodiscard]] audio::pcm_format pcm_format() const;
    [[nodiscard]] std::size_t channel_count() const;

    [[nodiscard]] chaining::chain_sync_t<audio_configuration> chain_configuration() const;
    [[nodiscard]] chaining::chain_sync_t<bool> chain_is_playing() const;
    [[nodiscard]] state_map_vector_holder_t::chain_t chain_state() const;

    static audio_coordinator_ptr make_shared(std::string const &root_path, audio::io_device_ptr const &);

   private:
    audio::io_device_ptr _device;
    std::string _root_path;
    std::shared_ptr<task_queue> _queue = std::make_shared<task_queue>();
    audio_renderer_ptr _renderer = audio_renderer::make_shared(this->_device);
    audio_player_ptr _player{audio_player::make_shared(this->_renderer, this->_root_path, this->_queue, 0)};

    chaining::observer_pool _pool;

    explicit audio_coordinator(std::string const &root_path, audio::io_device_ptr const &);

    audio_coordinator(audio_coordinator const &) = delete;
    audio_coordinator(audio_coordinator &&) = delete;
    audio_coordinator &operator=(audio_coordinator const &) = delete;
    audio_coordinator &operator=(audio_coordinator &&) = delete;
};
}  // namespace yas::playing
