//
//  yas_playing_renderer.cpp
//

#include "yas_playing_renderer.h"

#include <audio/yas_audio_graph_io.h>
#include <audio/yas_audio_io.h>

using namespace yas;
using namespace yas::playing;

renderer::renderer(audio::io_device_ptr const &device) : graph(audio::graph::make_shared()), _device(device) {
    this->_update_configuration();

    this->_sample_rate->chain()
        .to_null()
        .merge(this->_pcm_format->chain().to_null())
        .merge(this->_channel_count->chain().to_null())
        .perform([this](auto const &) {
            this->_configuration->set_value(configuration{.sample_rate = this->_sample_rate->raw(),
                                                          .pcm_format = this->_pcm_format->raw(),
                                                          .channel_count = this->_channel_count->raw()});
        })
        .sync()
        ->add_to(this->_pool);

    this->_device->io_device_chain()
        .perform([this](auto const &) { this->_update_configuration(); })
        .end()
        ->add_to(this->_pool);

    this->_configuration->chain()
        .perform([this](auto const &) { this->_update_connection(); })
        .sync()
        ->add_to(this->_pool);

    this->_is_rendering->chain()
        .perform([this](bool const &is_rendering) {
            if (is_rendering) {
                this->graph->start_render();
            } else {
                this->graph->stop();
            }
        })
        .sync()
        ->add_to(this->_pool);
}

proc::sample_rate_t renderer::sample_rate() const {
    return this->_sample_rate->raw();
}

audio::pcm_format renderer::pcm_format() const {
    return this->_pcm_format->raw();
}

std::size_t renderer::channel_count() const {
    return this->_channel_count->raw();
}

chaining::chain_sync_t<configuration> renderer::configuration_chain() const {
    return this->_configuration->chain();
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

chaining::chain_sync_t<proc::sample_rate_t> renderer::chain_sample_rate() {
    return this->_sample_rate->chain();
}

chaining::chain_sync_t<audio::pcm_format> renderer::chain_pcm_format() {
    return this->_pcm_format->chain();
}

chaining::chain_sync_t<std::size_t> renderer::chain_channel_count() {
    return this->_channel_count->chain();
}

void renderer::set_is_rendering(bool const is_rendering) {
    this->_is_rendering->set_value(is_rendering);
}

void renderer::_update_configuration() {
    if (auto const &device = this->_io->raw_io()->device()) {
        if (auto const &output_format = device.value()->output_format()) {
            this->_sample_rate->set_value(output_format->sample_rate());
            this->_channel_count->set_value(output_format->channel_count());
            return;
        }
    }
    this->_sample_rate->set_value(0);
    this->_channel_count->set_value(0);
}

void renderer::_update_connection() {
    if (this->_connection) {
        this->graph->disconnect(*this->_connection);
        this->_connection = std::nullopt;
    }

    double const &sample_rate = this->_sample_rate->raw();
    std::size_t const ch_count = this->_channel_count->raw();

    if (sample_rate > 0.0 && ch_count > 0) {
        audio::format format{{.sample_rate = sample_rate, .channel_count = static_cast<uint32_t>(ch_count)}};
        this->_connection = this->graph->connect(this->_tap->node, this->_io->output_node, format);
    }
}

renderer_ptr renderer::make_shared(audio::io_device_ptr const &device) {
    return renderer_ptr(new renderer{device});
}