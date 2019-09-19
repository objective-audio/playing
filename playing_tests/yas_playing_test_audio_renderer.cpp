//
//  yas_playing_test_audio_renderer.cpp
//

#include "yas_playing_test_audio_renderer.h"
#include <audio/yas_audio_format.h>
#include <processing/yas_processing_types.h>

using namespace yas;
using namespace yas::playing;
using namespace yas::playing::test_utils;

struct test_audio_renderer::impl {
    chaining::value::holder_ptr<proc::sample_rate_t> _sample_rate =
        chaining::value::holder<proc::sample_rate_t>::make_shared(proc::sample_rate_t{0});
    chaining::value::holder_ptr<audio::pcm_format> _pcm_format =
        chaining::value::holder<audio::pcm_format>::make_shared(audio::pcm_format::float32);
    chaining::value::holder_ptr<std::size_t> _channel_count =
        chaining::value::holder<std::size_t>::make_shared(std::size_t(0));
    std::atomic<bool> _is_rendering = false;
    audio_renderable::rendering_f _rendering_handler;
    std::recursive_mutex _rendering_mutex;

    void set_rendering_handler(audio_renderable::rendering_f &&handler) {
        std::lock_guard<std::recursive_mutex> lock(this->_rendering_mutex);
        this->_rendering_handler = std::move(handler);
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
        this->_is_rendering = is_rendering;
    }

    void render(audio::pcm_buffer &buffer) {
        if (!this->_is_rendering.load()) {
            return;
        }

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

test_audio_renderer::test_audio_renderer() : _impl(std::make_unique<impl>()) {
}

void test_audio_renderer::set_pcm_format(audio::pcm_format const pcm_fomat) {
    this->_impl->_pcm_format->set_value(pcm_fomat);
}

void test_audio_renderer::set_channel_count(uint32_t const ch_count) {
    this->_impl->_channel_count->set_value(ch_count);
}

void test_audio_renderer::set_sample_rate(double const sample_rate) {
    this->_impl->_sample_rate->set_value(sample_rate);
}

void test_audio_renderer::render(audio::pcm_buffer &buffer) {
    this->_impl->render(buffer);
}

void test_audio_renderer::set_rendering_handler(rendering_f handler) {
    this->_impl->set_rendering_handler(std::move(handler));
}

chaining::chain_sync_t<proc::sample_rate_t> test_audio_renderer::chain_sample_rate() {
    return this->_impl->chain_sample_rate();
}

chaining::chain_sync_t<audio::pcm_format> test_audio_renderer::chain_pcm_format() {
    return this->_impl->chain_pcm_format();
}

chaining::chain_sync_t<std::size_t> test_audio_renderer::chain_channel_count() {
    return this->_impl->chain_channel_count();
}

void test_audio_renderer::set_is_rendering(bool const is_rendering) {
    this->_impl->set_is_rendering(is_rendering);
}

test_audio_renderer_ptr test_audio_renderer::make_shared() {
    return test_audio_renderer_ptr(new test_audio_renderer{});
}
