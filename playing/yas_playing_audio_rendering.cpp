//
//  yas_playing_audio_rendering.cpp
//

#include "yas_playing_audio_rendering.h"

#include <cpp_utils/yas_fast_each.h>

#include "yas_playing_audio_buffering.h"
#include "yas_playing_audio_reading.h"

using namespace yas;
using namespace yas::playing;

audio_rendering::audio_rendering() {
}

void audio_rendering::set_is_playing_on_main(bool const is_playing) {
    this->_is_playing.store(is_playing);
}

bool audio_rendering::is_playing_on_render() const {
    return this->_is_playing.load();
}

void audio_rendering::seek_on_main(frame_index_t const frame) {
    std::lock_guard<std::recursive_mutex> lock(this->_seek_mutex);
    this->_seek_frame = frame;
}

std::optional<frame_index_t> audio_rendering::pull_seek_frame_on_render() {
    if (auto lock = std::unique_lock<std::recursive_mutex>(this->_seek_mutex, std::try_to_lock); lock.owns_lock()) {
        if (this->_seek_frame.has_value()) {
            auto frame = this->_seek_frame;
            this->_seek_frame = std::nullopt;
            return frame;
        }
    }
    return std::nullopt;
}

void audio_rendering::set_ch_mapping_on_main(std::vector<channel_index_t> const &ch_mapping) {
    std::lock_guard<std::recursive_mutex> lock(this->_ch_mapping_mutex);
    this->_ch_mapping = ch_mapping;
    this->_ch_mapping_changed = true;
}

std::optional<std::vector<channel_index_t>> audio_rendering::pull_ch_mapping_on_render() {
    if (auto lock = std::unique_lock<std::recursive_mutex>(this->_ch_mapping_mutex, std::try_to_lock);
        lock.owns_lock()) {
        if (this->_ch_mapping_changed) {
            this->_ch_mapping_changed = false;
            return this->_ch_mapping;
        }
    }
    return std::nullopt;
}

void audio_rendering::set_play_frame_on_render(frame_index_t const frame) {
    this->_play_frame = frame;
}

frame_index_t audio_rendering::play_frame_on_main() const {
    return this->_play_frame.load();
}

frame_index_t audio_rendering::play_frame_on_render() const {
    return this->_play_frame.load();
}

void audio_rendering::add_overwrite_request_on_main(element_address &&request) {
    std::lock_guard<std::recursive_mutex> lock(this->_overwrite_mutex);
    if (this->_is_overwritten) {
        this->_overwrite_requests.clear();
        this->_is_overwritten = false;
    }
    this->_overwrite_requests.emplace_back(std::move(request));
}

void audio_rendering::perform_overwrite_requests_on_render(overwrite_requests_f const &handler) {
    if (auto lock = std::unique_lock<std::recursive_mutex>(this->_overwrite_mutex, std::try_to_lock);
        lock.owns_lock()) {
        if (!this->_is_overwritten) {
            handler(this->_overwrite_requests);
            this->_is_overwritten = true;
        }
    }
}

void audio_rendering::reset_overwrite_requests_on_render() {
    if (auto lock = std::unique_lock<std::recursive_mutex>(this->_overwrite_mutex, std::try_to_lock);
        lock.owns_lock()) {
        this->_is_overwritten = true;
    }
}

audio_rendering_ptr audio_rendering::make_shared() {
    return audio_rendering_ptr{new audio_rendering{}};
}
