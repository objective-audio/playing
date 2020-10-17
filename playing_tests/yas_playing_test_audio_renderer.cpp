//
//  yas_playing_test_audio_renderer.cpp
//

#include "yas_playing_test_audio_renderer.h"

#include <audio/yas_audio_format.h>
#include <cpp_utils/yas_thread.h>
#include <processing/yas_processing_types.h>

using namespace yas;
using namespace yas::playing;
using namespace yas::playing::test_utils;

test_audio_renderer::test_audio_renderer() {
}

void test_audio_renderer::set_pcm_format(audio::pcm_format const pcm_fomat) {
    this->_pcm_format->set_value(pcm_fomat);
}

void test_audio_renderer::set_channel_count(uint32_t const ch_count) {
    this->_channel_count->set_value(ch_count);
}

void test_audio_renderer::set_sample_rate(double const sample_rate) {
    this->_sample_rate->set_value(sample_rate);
}

bool test_audio_renderer::render_on_bg(audio::pcm_buffer *const buffer) {
    assert(!thread::is_main());

    if (!this->_is_rendering.load()) {
        return false;
    }

    auto const &format = buffer->format();

    if (format.is_interleaved()) {
        return false;
    }

    if (auto const &handler = this->_rendering_handler) {
        handler(buffer);
        return true;
    } else {
        return false;
    }
}

void test_audio_renderer::set_rendering_handler(rendering_f &&handler) {
    this->_rendering_handler = std::move(handler);
}

chaining::chain_sync_t<proc::sample_rate_t> test_audio_renderer::chain_sample_rate() {
    return this->_sample_rate->chain();
}

chaining::chain_sync_t<audio::pcm_format> test_audio_renderer::chain_pcm_format() {
    return this->_pcm_format->chain();
}

chaining::chain_sync_t<std::size_t> test_audio_renderer::chain_channel_count() {
    return this->_channel_count->chain();
}

void test_audio_renderer::set_is_rendering(bool const is_rendering) {
    this->_is_rendering = is_rendering;
}

test_audio_renderer_ptr test_audio_renderer::make_shared() {
    return test_audio_renderer_ptr(new test_audio_renderer{});
}
