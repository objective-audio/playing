//
//  yas_playing_renderer.cpp
//

#include "yas_playing_renderer.h"

#include <audio/yas_audio_graph_io.h>
#include <audio/yas_audio_io.h>
#include <playing/yas_playing_types.h>

using namespace yas;
using namespace yas::playing;

renderer::renderer(audio::io_device_ptr const &device) : graph(audio::graph::make_shared()), _device(device) {
    this->_update_configuration();

    auto set_config_handler = [this] {
        this->_configuration->set_value(playing::configuration{.sample_rate = this->_sample_rate->value(),
                                                               .pcm_format = this->_pcm_format->value(),
                                                               .channel_count = this->_channel_count->value()});
    };

    this->_sample_rate->observe([set_config_handler](auto const &) { set_config_handler(); })
        .end()
        ->add_to(this->_pool);

    this->_pcm_format->observe([set_config_handler](auto const &) { set_config_handler(); }).end()->add_to(this->_pool);

    this->_channel_count->observe([set_config_handler](auto const &) { set_config_handler(); })
        .sync()
        ->add_to(this->_pool);

    this->_device->observe_io_device([this](auto const &) { this->_update_configuration(); })
        .end()
        ->add_to(this->_pool);

    this->_configuration->observe([this](auto const &) { this->_update_connection(); }).sync()->add_to(this->_pool);

    this->_is_rendering
        ->observe([this](bool const &is_rendering) {
            if (is_rendering) {
                this->graph->start_render();
            } else {
                this->graph->stop();
            }
        })
        .sync()
        ->add_to(this->_pool);
}

configuration const &renderer::configuration() const {
    return this->_configuration->value();
}

observing::syncable renderer::observe_configuration(configuration_observing_handler_f &&handler) {
    return this->_configuration->observe(std::move(handler));
}

void renderer::set_rendering_handler(renderable::rendering_f &&handler) {
    this->_tap->set_render_handler([handler = std::move(handler)](audio::node_render_args const &args) {
        if (args.bus_idx != 0) {
            return;
        }

        auto const &buffer = args.buffer;

        if (buffer->format().is_interleaved()) {
            return;
        }

        if (handler) {
            handler(buffer);
        }
    });
}

void renderer::set_is_rendering(bool const is_rendering) {
    this->_is_rendering->set_value(is_rendering);
}

void renderer::_update_configuration() {
    if (auto const &output_format = this->_device->output_format()) {
        this->_sample_rate->set_value(output_format->sample_rate());
        this->_channel_count->set_value(output_format->channel_count());
        this->_pcm_format->set_value(output_format->pcm_format());
    } else {
        this->_sample_rate->set_value(0);
        this->_channel_count->set_value(0);
        this->_pcm_format->set_value(audio::pcm_format::other);
    }
}

void renderer::_update_connection() {
    if (this->_connection) {
        this->graph->disconnect(*this->_connection);
        this->_connection = std::nullopt;
    }

    sample_rate_t const &sample_rate = this->_sample_rate->value();
    std::size_t const ch_count = this->_channel_count->value();
    audio::pcm_format const pcm_format = this->_pcm_format->value();

    if (sample_rate > 0 && ch_count > 0 && pcm_format != audio::pcm_format::other) {
        audio::format const format{{.sample_rate = static_cast<double>(sample_rate),
                                    .channel_count = static_cast<uint32_t>(ch_count),
                                    .pcm_format = pcm_format}};
        this->_connection = this->graph->connect(this->_tap->node, this->_io->output_node, format);
    }
}

renderer_ptr renderer::make_shared(audio::io_device_ptr const &device) {
    return renderer_ptr(new renderer{device});
}
