//
//  yas_playing_audio_renderer.cpp
//

#include "yas_playing_audio_renderer.h"
#include <audio/yas_audio_engine_au.h>
#include <audio/yas_audio_engine_au_io.h>
#include <audio/yas_audio_engine_tap.h>
#include <audio/yas_audio_format.h>

using namespace yas;
using namespace yas::playing;

struct audio_renderer::impl {
    audio::engine::manager_ptr _manager = audio::engine::manager::make_shared();
    chaining::value::holder_ptr<proc::sample_rate_t> _sample_rate =
        chaining::value::holder<proc::sample_rate_t>::make_shared(proc::sample_rate_t{0});
    chaining::value::holder_ptr<audio::pcm_format> _pcm_format =
        chaining::value::holder<audio::pcm_format>::make_shared(audio::pcm_format::float32);
    chaining::value::holder_ptr<std::size_t> _channel_count =
        chaining::value::holder<std::size_t>::make_shared(std::size_t(0));
    chaining::value::holder_ptr<audio_configuration> _configuration =
        chaining::value::holder<audio_configuration>::make_shared(
            {.sample_rate = 0, .pcm_format = audio::pcm_format::float32, .channel_count = 0});

    void prepare(audio_renderer_ptr const &renderer) {
        auto weak_renderer = to_weak(renderer);

        this->_setup_tap(weak_renderer);

        this->_pool += this->_sample_rate->chain()
                           .to_null()
                           .merge(this->_pcm_format->chain().to_null())
                           .merge(this->_channel_count->chain().to_null())
                           .perform([weak_renderer](auto const &) {
                               if (auto renderer = weak_renderer.lock()) {
                                   auto const &renderer_impl = renderer->_impl;
                                   renderer_impl->_configuration->set_value(
                                       audio_configuration{.sample_rate = renderer_impl->_sample_rate->raw(),
                                                           .pcm_format = renderer_impl->_pcm_format->raw(),
                                                           .channel_count = renderer_impl->_channel_count->raw()});
                               }
                           })
                           .sync();

        this->_update_configuration();

        this->_pool += this->_manager->chain(audio::engine::manager::method::configuration_change)
                           .perform([weak_renderer](auto const &) {
                               if (auto renderer = weak_renderer.lock()) {
                                   renderer->_impl->_update_configuration();
                               }
                           })
                           .end();

        this->_pool += this->_configuration->chain()
                           .perform([weak_renderer](auto const &) {
                               if (auto renderer = weak_renderer.lock()) {
                                   renderer->_impl->_update_connection();
                               }
                           })
                           .sync();

        this->_pool += this->_is_rendering->chain()
                           .perform([weak_renderer](bool const &is_rendering) {
                               if (auto renderer = weak_renderer.lock()) {
                                   if (is_rendering) {
                                       renderer->_impl->_manager->start_render();
                                   } else {
                                       renderer->_impl->_manager->stop();
                                   }
                               }
                           })
                           .sync();
    }

    void set_rendering_handler(audio_renderable::rendering_f &&handler) {
        std::lock_guard<std::recursive_mutex> lock(this->_rendering_mutex);
        this->_rendering_handler = handler;
    }

    chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() {
        return this->_sample_rate->chain();
    }

    chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() {
        return this->_pcm_format->chain();
    }

    chaining::chain_sync_t<std::size_t> chain_channel_count() {
        return this->_channel_count->chain();
    }

    void set_is_rendering(bool const is_rendering) {
        this->_is_rendering->set_value(is_rendering);
    }

   private:
    audio::engine::au_output_ptr _output = audio::engine::au_output::make_shared();
    audio::engine::tap_ptr _tap = audio::engine::tap::make_shared();
    audio::engine::connection_ptr _connection = nullptr;

    chaining::observer_pool _pool;

    chaining::value::holder_ptr<bool> _is_rendering = chaining::value::holder<bool>::make_shared(false);
    audio_renderable::rendering_f _rendering_handler;
    std::recursive_mutex _rendering_mutex;

    void _setup_tap(std::weak_ptr<audio_renderer> const &weak_renderer) {
        this->_tap->set_render_handler(
            [weak_renderer = std::move(weak_renderer)](audio::engine::node::render_args args) {
                if (args.bus_idx != 0) {
                    return;
                }

                if (auto renderer = weak_renderer.lock()) {
                    renderer->_impl->_render(args.buffer);
                }
            });
    }

    void _update_configuration() {
        audio::engine::au_io const &au_io = this->_output->au_io();
        proc::sample_rate_t const sample_rate =
            static_cast<proc::sample_rate_t>(std::round(au_io.device_sample_rate()));
        uint32_t const ch_count = au_io.output_device_channel_count();
        this->_sample_rate->set_value(sample_rate);
        this->_channel_count->set_value(ch_count);
    }

    void _update_connection() {
        if (this->_connection) {
            this->_manager->disconnect(this->_connection);
            this->_connection = nullptr;
        }

        double const &sample_rate = this->_sample_rate->raw();
        std::size_t const ch_count = this->_channel_count->raw();

        if (sample_rate > 0.0 && ch_count > 0) {
            audio::format format{{.sample_rate = sample_rate, .channel_count = static_cast<uint32_t>(ch_count)}};
            this->_connection = this->_manager->connect(this->_tap->node(), this->_output->au_io().au().node(), format);
        }
    }

    void _render(audio::pcm_buffer &buffer) {
        auto const &format = buffer.format();

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
};

audio_renderer::audio_renderer() : _impl(std::make_unique<impl>()) {
}

audio::engine::manager_ptr const &audio_renderer::manager() {
    return this->_impl->_manager;
}

proc::sample_rate_t audio_renderer::sample_rate() const {
    return this->_impl->_sample_rate->raw();
}

audio::pcm_format audio_renderer::pcm_format() const {
    return this->_impl->_pcm_format->raw();
}

std::size_t audio_renderer::channel_count() const {
    return this->_impl->_channel_count->raw();
}

chaining::chain_sync_t<audio_configuration> audio_renderer::configuration_chain() const {
    return this->_impl->_configuration->chain();
}

void audio_renderer::_prepare(audio_renderer_ptr const &shared) {
    this->_impl->prepare(shared);
}

void audio_renderer::set_rendering_handler(audio_renderable::rendering_f handler) {
    this->_impl->set_rendering_handler(std::move(handler));
}

chaining::chain_sync_t<proc::sample_rate_t> audio_renderer::chain_sample_rate() {
    return this->_impl->chain_sample_rate();
}

chaining::chain_sync_t<audio::pcm_format> audio_renderer::chain_pcm_format() {
    return this->_impl->chain_pcm_format();
}

chaining::chain_sync_t<std::size_t> audio_renderer::chain_channel_count() {
    return this->_impl->chain_channel_count();
}

void audio_renderer::set_is_rendering(bool const is_rendering) {
    this->_impl->set_is_rendering(is_rendering);
}

audio_renderer_ptr audio_renderer::make_shared() {
    auto shared = audio_renderer_ptr(new audio_renderer{});
    shared->_prepare(shared);
    return shared;
}
