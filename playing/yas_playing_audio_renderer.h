//
//  yas_playing_audio_renderer.h
//

#pragma once

#include <audio/yas_audio_engine_manager.h>
#include <processing/yas_processing_types.h>
#include "yas_playing_audio_configulation.h"
#include "yas_playing_audio_player_protocol.h"
#include "yas_playing_ptr.h"

namespace yas::playing {
struct audio_renderer : audio_renderable {
    [[nodiscard]] audio::engine::manager_ptr const &manager();
    [[nodiscard]] proc::sample_rate_t sample_rate() const;
    [[nodiscard]] audio::pcm_format pcm_format() const;
    [[nodiscard]] std::size_t channel_count() const;

    [[nodiscard]] chaining::chain_sync_t<audio_configuration> configuration_chain() const;

    static audio_renderer_ptr make_shared();

   private:
    class impl;

    std::unique_ptr<impl> _impl;

    audio_renderer();

    void _prepare(audio_renderer_ptr const &);

    void set_rendering_handler(audio_renderable::rendering_f) override;
    chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() override;
    chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() override;
    chaining::chain_sync_t<std::size_t> chain_channel_count() override;
    void set_is_rendering(bool const) override;
};
}  // namespace yas::playing
