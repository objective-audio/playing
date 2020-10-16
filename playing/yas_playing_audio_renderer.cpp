//
//  yas_playing_audio_renderer.cpp
//

#include "yas_playing_audio_renderer.h"

#include <audio/yas_audio_graph_io.h>
#include <audio/yas_audio_io.h>

using namespace yas;
using namespace yas::playing;

audio_renderer::audio_renderer(audio::io_device_ptr const &device)
    : graph(audio::graph::make_shared()), _device(device) {
    this->_update_configuration();

    this->_sample_rate->chain()
        .to_null()
        .merge(this->_pcm_format->chain().to_null())
        .merge(this->_channel_count->chain().to_null())
        .perform([this](auto const &) {
            this->_configuration->set_value(audio_configuration{.sample_rate = this->_sample_rate->raw(),
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

proc::sample_rate_t audio_renderer::sample_rate() const {
    return this->_sample_rate->raw();
}

audio::pcm_format audio_renderer::pcm_format() const {
    return this->_pcm_format->raw();
}

std::size_t audio_renderer::channel_count() const {
    return this->_channel_count->raw();
}

chaining::chain_sync_t<audio_configuration> audio_renderer::configuration_chain() const {
    return this->_configuration->chain();
}

void audio_renderer::set_rendering_handler(audio_renderable::rendering_f handler) {
    std::lock_guard<std::recursive_mutex> lock(this->_rendering_mutex);
    this->_rendering_handler = std::move(handler);
}

chaining::chain_sync_t<proc::sample_rate_t> audio_renderer::chain_sample_rate() {
    return this->_sample_rate->chain();
}

chaining::chain_sync_t<audio::pcm_format> audio_renderer::chain_pcm_format() {
    return this->_pcm_format->chain();
}

chaining::chain_sync_t<std::size_t> audio_renderer::chain_channel_count() {
    return this->_channel_count->chain();
}

void audio_renderer::set_is_rendering(bool const is_rendering) {
    this->_is_rendering->set_value(is_rendering);
}

void audio_renderer::_update_tap_renderer() {
    this->_tap->set_render_handler([handler = this->_rendering_handler](audio::node_render_args const &args) {
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

void audio_renderer::_update_configuration() {
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

void audio_renderer::_update_connection() {
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

void audio_renderer::_render(audio::pcm_buffer *const buffer) {
    auto const &format = buffer->format();

    if (format.is_interleaved()) {
        throw std::invalid_argument("buffer is not non-interleaved.");
    }

    if (auto lock = std::unique_lock<std::recursive_mutex>(this->_rendering_mutex, std::try_to_lock);
        lock.owns_lock()) {
        if (auto const &handler = this->_rendering_handler) {
            handler(buffer);
        }
    }
}

audio_renderer_ptr audio_renderer::make_shared(audio::io_device_ptr const &device) {
    return audio_renderer_ptr(new audio_renderer{device});
}
