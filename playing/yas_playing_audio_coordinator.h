//
//  yas_playing_audio_coordinator.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_worker.h>
#include <playing/yas_playing_audio_configulation.h>
#include <playing/yas_playing_audio_player.h>
#include <playing/yas_playing_audio_renderer.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_time.h>

namespace yas::playing {
struct audio_coordinator {
    void set_channel_mapping(std::vector<channel_index_t>);
    void set_playing(bool const);
    void seek(frame_index_t const);
    void overwrite(proc::time::range const &);

    [[nodiscard]] bool is_playing() const;
    [[nodiscard]] frame_index_t current_frame() const;

    [[nodiscard]] sample_rate_t sample_rate() const;
    [[nodiscard]] audio::pcm_format pcm_format() const;
    [[nodiscard]] std::size_t channel_count() const;

    [[nodiscard]] chaining::chain_sync_t<audio_configuration> configuration_chain() const;
    [[nodiscard]] chaining::chain_sync_t<bool> is_playing_chain() const;

    [[nodiscard]] static audio_coordinator_ptr make_shared(std::string const &root_path, audio::io_device_ptr const &);

   private:
    audio::io_device_ptr const _device;
    std::string const _root_path;
    worker_ptr const _worker = worker::make_shared();
    audio_coordinator_renderable_ptr const _renderer = audio_renderer::make_shared(this->_device);
    audio_playable_ptr const _player;

    chaining::observer_pool _pool;

    explicit audio_coordinator(std::string const &root_path, audio::io_device_ptr const &);

    audio_coordinator(audio_coordinator const &) = delete;
    audio_coordinator(audio_coordinator &&) = delete;
    audio_coordinator &operator=(audio_coordinator const &) = delete;
    audio_coordinator &operator=(audio_coordinator &&) = delete;
};
}  // namespace yas::playing
