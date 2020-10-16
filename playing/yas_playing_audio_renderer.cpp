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

void audio_renderer::_prepare(audio_renderer_ptr const &renderer) {
    auto weak_renderer = to_weak(renderer);

    this->_setup_tap(weak_renderer);

    this->_pool += this->_sample_rate->chain()
                       .to_null()
                       .merge(this->_pcm_format->chain().to_null())
                       .merge(this->_channel_count->chain().to_null())
                       .perform([weak_renderer](auto const &) {
                           if (auto renderer = weak_renderer.lock()) {
                               renderer->_configuration->set_value(
                                   audio_configuration{.sample_rate = renderer->_sample_rate->raw(),
                                                       .pcm_format = renderer->_pcm_format->raw(),
                                                       .channel_count = renderer->_channel_count->raw()});
                           }
                       })
                       .sync();

    this->_update_configuration();

    this->_pool += this->_device->io_device_chain()
                       .perform([weak_renderer](auto const &) {
                           if (auto renderer = weak_renderer.lock()) {
                               renderer->_update_configuration();
                           }
                       })
                       .end();

    this->_pool += this->_configuration->chain()
                       .perform([weak_renderer](auto const &) {
                           if (auto renderer = weak_renderer.lock()) {
                               renderer->_update_connection();
                           }
                       })
                       .sync();

    this->_pool += this->_is_rendering->chain()
                       .perform([weak_renderer](bool const &is_rendering) {
                           if (auto renderer = weak_renderer.lock()) {
                               if (is_rendering) {
                                   renderer->graph->start_render();
                               } else {
                                   renderer->graph->stop();
                               }
                           }
                       })
                       .sync();
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

void audio_renderer::_setup_tap(std::weak_ptr<audio_renderer> const &weak_renderer) {
    this->_tap->set_render_handler([weak_renderer = std::move(weak_renderer)](audio::node_render_args const &args) {
        if (args.bus_idx != 0) {
            return;
        }

        if (auto renderer = weak_renderer.lock()) {
            renderer->_render(args.buffer);
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
    auto shared = audio_renderer_ptr(new audio_renderer{device});
    shared->_prepare(shared);
    return shared;
}
