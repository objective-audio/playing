//
//  yas_playing_renderer.h
//

#pragma once

#include <audio/yas_audio_graph.h>
#include <audio/yas_audio_graph_tap.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_renderer_protocol.h>
#include <processing/yas_processing_common_types.h>

namespace yas::playing {
struct renderer final : coordinator_renderable {
    audio::graph_ptr const graph;

    void set_rendering_handler(renderable::rendering_f &&) override;
    void set_is_rendering(bool const) override;

    [[nodiscard]] playing::configuration const &configuration() const override;
    [[nodiscard]] sample_rate_t sample_rate() const override;
    [[nodiscard]] audio::pcm_format pcm_format() const override;
    [[nodiscard]] std::size_t channel_count() const override;

    [[nodiscard]] observing::syncable observe_configuration(configuration_observing_handler_f &&) override;

    static renderer_ptr make_shared(audio::io_device_ptr const &);

   private:
    audio::io_device_ptr const _device;

    observing::value::holder_ptr<sample_rate_t> const _sample_rate =
        observing::value::holder<sample_rate_t>::make_shared(sample_rate_t{0});
    observing::value::holder_ptr<audio::pcm_format> const _pcm_format =
        observing::value::holder<audio::pcm_format>::make_shared(audio::pcm_format::float32);
    observing::value::holder_ptr<std::size_t> const _channel_count =
        observing::value::holder<std::size_t>::make_shared(std::size_t(0));
    observing::value::holder_ptr<playing::configuration> const _configuration =
        observing::value::holder<playing::configuration>::make_shared(
            {.sample_rate = 0, .pcm_format = audio::pcm_format::float32, .channel_count = 0});

    audio::graph_io_ptr const _io = this->graph->add_io(this->_device);
    audio::graph_tap_ptr const _tap = audio::graph_tap::make_shared();
    std::optional<audio::graph_connection_ptr> _connection = std::nullopt;

    observing::canceller_pool _pool;

    observing::value::holder_ptr<bool> _is_rendering = observing::value::holder<bool>::make_shared(false);

    renderer(audio::io_device_ptr const &);

    void _update_configuration();
    void _update_connection();
};
}  // namespace yas::playing
