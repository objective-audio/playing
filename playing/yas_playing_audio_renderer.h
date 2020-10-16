//
//  yas_playing_audio_renderer.h
//

#pragma once

#include <audio/yas_audio_graph.h>
#include <audio/yas_audio_graph_tap.h>
#include <processing/yas_processing_types.h>

#include "yas_playing_audio_configulation.h"
#include "yas_playing_audio_player_protocol.h"
#include "yas_playing_ptr.h"

namespace yas::playing {
struct audio_renderer : audio_renderable {
    audio::graph_ptr const graph;

    [[nodiscard]] proc::sample_rate_t sample_rate() const;
    [[nodiscard]] audio::pcm_format pcm_format() const;
    [[nodiscard]] std::size_t channel_count() const;

    [[nodiscard]] chaining::chain_sync_t<audio_configuration> configuration_chain() const;

    static audio_renderer_ptr make_shared(audio::io_device_ptr const &);

   private:
    audio::io_device_ptr const _device;

    chaining::value::holder_ptr<proc::sample_rate_t> const _sample_rate =
        chaining::value::holder<proc::sample_rate_t>::make_shared(proc::sample_rate_t{0});
    chaining::value::holder_ptr<audio::pcm_format> const _pcm_format =
        chaining::value::holder<audio::pcm_format>::make_shared(audio::pcm_format::float32);
    chaining::value::holder_ptr<std::size_t> const _channel_count =
        chaining::value::holder<std::size_t>::make_shared(std::size_t(0));
    chaining::value::holder_ptr<audio_configuration> const _configuration =
        chaining::value::holder<audio_configuration>::make_shared(
            {.sample_rate = 0, .pcm_format = audio::pcm_format::float32, .channel_count = 0});

    audio::graph_io_ptr const _io = this->graph->add_io(this->_device);
    audio::graph_tap_ptr const _tap = audio::graph_tap::make_shared();
    std::optional<audio::graph_connection_ptr> _connection = std::nullopt;

    chaining::observer_pool _pool;

    chaining::value::holder_ptr<bool> _is_rendering = chaining::value::holder<bool>::make_shared(false);

    audio_renderer(audio::io_device_ptr const &);

    void set_rendering_handler(audio_renderable::rendering_f) override;
    chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() override;
    chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() override;
    chaining::chain_sync_t<std::size_t> chain_channel_count() override;
    void set_is_rendering(bool const) override;

    void _update_configuration();
    void _update_connection();
};
}  // namespace yas::playing
