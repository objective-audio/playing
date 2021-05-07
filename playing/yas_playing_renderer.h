//
//  yas_playing_renderer.h
//

#pragma once

#include <audio/yas_audio_graph.h>
#include <audio/yas_audio_graph_avf_au.h>
#include <audio/yas_audio_graph_tap.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_renderer_protocol.h>
#include <processing/yas_processing_common_types.h>

namespace yas::playing {
struct renderer final : coordinator_renderable {
    audio::graph_ptr const graph;

    void set_rendering_sample_rate(sample_rate_t const) override;
    void set_rendering_pcm_format(audio::pcm_format const) override;
    void set_rendering_handler(renderable::rendering_f &&) override;
    void set_is_rendering(bool const) override;

    [[nodiscard]] playing::configuration const &configuration() const override;

    [[nodiscard]] observing::syncable observe_configuration(configuration_observing_handler_f &&) override;

    static renderer_ptr make_shared(audio::io_device_ptr const &);

   private:
    audio::io_device_ptr const _device;

    observing::value::holder_ptr<sample_rate_t> const _rendering_sample_rate;
    observing::value::holder_ptr<audio::pcm_format> const _rendering_pcm_format;

    observing::value::holder_ptr<sample_rate_t> const _output_sample_rate;
    observing::value::holder_ptr<audio::pcm_format> const _output_pcm_format;

    observing::value::holder_ptr<sample_rate_t> const _config_sample_rate;
    observing::value::holder_ptr<audio::pcm_format> const _config_pcm_format;
    observing::value::holder_ptr<std::size_t> const _config_channel_count;
    observing::value::holder_ptr<playing::configuration> const _configuration;

    audio::graph_io_ptr const _io;
    audio::graph_avf_au_ptr const _converter;
    audio::graph_tap_ptr const _tap;
    std::optional<audio::graph_connection_ptr> _connection{std::nullopt};
    std::optional<audio::graph_connection_ptr> _converter_connection{std::nullopt};

    observing::canceller_pool _pool;

    observing::value::holder_ptr<bool> _is_rendering = observing::value::holder<bool>::make_shared(false);

    renderer(audio::io_device_ptr const &);

    void _update_configuration();
    void _update_connection();
};
}  // namespace yas::playing
