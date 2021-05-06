//
//  yas_playing_coordinator.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <cpp_utils/yas_worker.h>
#include <playing/yas_playing_configulation.h>
#include <playing/yas_playing_player.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_renderer.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_time.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct coordinator final {
    void set_timeline(proc::timeline_ptr const &, std::string const &identifier);
    void reset_timeline();
    void set_channel_mapping(channel_mapping const &);
    void set_playing(bool const);
    void seek(frame_index_t const);
    void overwrite(proc::time::range const &);

    [[nodiscard]] std::string const &identifier() const;
    [[nodiscard]] std::optional<proc::timeline_ptr> const &timeline() const;
    [[nodiscard]] channel_mapping channel_mapping() const;
    [[nodiscard]] bool is_playing() const;
    [[nodiscard]] frame_index_t current_frame() const;

    [[nodiscard]] playing::configuration const &configuration() const;

    [[nodiscard]] observing::syncable observe_configuration(std::function<void(playing::configuration const &)> &&);
    [[nodiscard]] observing::syncable observe_is_playing(std::function<void(bool const &)> &&);

    [[nodiscard]] static coordinator_ptr make_shared(std::string const &root_path, coordinator_renderable_ptr const &);
    [[nodiscard]] static coordinator_ptr make_shared(workable_ptr const &, coordinator_renderable_ptr const &,
                                                     playable_ptr const &, exportable_ptr const &);

   private:
    workable_ptr const _worker = worker::make_shared();
    coordinator_renderable_ptr const _renderer;
    playable_ptr const _player;
    exportable_ptr const _exporter;
    std::string _identifier = "";
    std::optional<proc::timeline_ptr> _timeline = std::nullopt;

    observing::canceller_pool _pool;

    coordinator(workable_ptr const &, coordinator_renderable_ptr const &, playable_ptr const &, exportable_ptr const &);

    void _update_exporter();
};
}  // namespace yas::playing
