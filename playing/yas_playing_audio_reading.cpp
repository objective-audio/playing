//
//  yas_playing_audio_reading.cpp
//

#include "yas_playing_audio_reading.h"

#include <thread>

using namespace yas;
using namespace yas::playing;

audio_reading::audio_reading() {
}

audio_reading::state_t audio_reading::state() const {
    return this->_current_state.load();
}

audio::pcm_buffer *audio_reading::buffer_on_render() {
    if (this->_current_state != state_t::rendering) {
        throw std::runtime_error("state is not rendering");
    }

    return this->_buffer.get();
}

bool audio_reading::needs_create_on_render(double const sample_rate, audio::pcm_format const pcm_format,
                                           uint32_t const length) const {
    if (this->_current_state != state_t::rendering) {
        throw std::runtime_error("state is not rendering");
    }

    return this->_buffer->format().sample_rate() != sample_rate || this->_buffer->frame_capacity() < length;
}

void audio_reading::set_creating_on_render(double const sample_rate, audio::pcm_format const pcm_format,
                                           uint32_t const length) {
    if (this->_current_state == state_t::creating) {
        throw std::runtime_error("state is already creating.");
    }

    if (length == 0) {
        throw std::invalid_argument("length is zero.");
    }

    this->_sample_rate = sample_rate;
    this->_pcm_format = pcm_format;
    this->_length = length;

    this->_current_state.store(state_t::creating);
}

void audio_reading::create_buffer_on_task() {
    if (this->_current_state != state_t::creating) {
        throw std::runtime_error("state is not creating.");
    }

    this->_buffer = nullptr;

    std::this_thread::yield();

    if (this->_sample_rate == 0) {
        throw std::runtime_error("sample_rate is zero.");
    }

    if (this->_length == 0) {
        throw std::runtime_error("length is zero.");
    }

    audio::format const format{
        {.sample_rate = this->_sample_rate, .pcm_format = this->_pcm_format, .interleaved = false, .channel_count = 1}};

    this->_buffer = std::make_shared<audio::pcm_buffer>(format, this->_length);
    this->_sample_rate = 0;
    this->_length = 0;

    this->_current_state.store(state_t::rendering);

    std::this_thread::yield();
}

audio_reading_ptr audio_reading::make_shared() {
    return audio_reading_ptr(new audio_reading{});
}
