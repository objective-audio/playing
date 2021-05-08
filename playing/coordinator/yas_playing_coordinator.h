//
//  yas_playing_coordinator.h
//

#pragma once

#include <audio/yas_audio_pcm_buffer.h>
#include <cpp_utils/yas_worker.h>
#include <playing/yas_playing_player.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_renderer.h>
#include <playing/yas_playing_renderer_types.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_time.h>
#include <processing/yas_processing_timeline.h>

namespace yas::playing {
struct coordinator final {
    void set_timeline(proc::timeline_ptr const &, std::string const &identifier);
    void reset_timeline();
    void set_timeline_format(sample_rate_t const, audio::pcm_format const);
    void set_channel_mapping(channel_mapping const &);
    void set_playing(bool const);
    void seek(frame_index_t const);
    void overwrite(proc::time::range const &);

    [[nodiscard]] std::string const &identifier() const;
    [[nodiscard]] std::optional<proc::timeline_ptr> const &timeline() const;
    [[nodiscard]] channel_mapping channel_mapping() const;
    [[nodiscard]] bool is_playing() const;
    [[nodiscard]] frame_index_t current_frame() const;

    [[nodiscard]] renderer_format const &format() const;

    [[nodiscard]] observing::syncable observe_format(std::function<void(renderer_format const &)> &&);
    [[nodiscard]] observing::syncable observe_is_playing(std::function<void(bool const &)> &&);

    [[nodiscard]] static coordinator_ptr make_shared(std::string const &root_path, std::shared_ptr<renderer> const &);
    [[nodiscard]] static coordinator_ptr make_shared(workable_ptr const &,
                                                     std::shared_ptr<coordinator_renderer_interface> const &,
                                                     std::shared_ptr<coordinator_player_interface> const &,
                                                     std::shared_ptr<coordinator_exporter_interface> const &);

   private:
    workable_ptr const _worker;
    std::shared_ptr<coordinator_renderer_interface> const _renderer;
    std::shared_ptr<coordinator_player_interface> const _player;
    std::shared_ptr<coordinator_exporter_interface> const _exporter;
    std::string _identifier = "";
    std::optional<proc::timeline_ptr> _timeline = std::nullopt;

    observing::canceller_pool _pool;

    coordinator(workable_ptr const &, std::shared_ptr<coordinator_renderer_interface> const &,
                std::shared_ptr<coordinator_player_interface> const &,
                std::shared_ptr<coordinator_exporter_interface> const &);

    void _update_exporter();
};
}  // namespace yas::playing