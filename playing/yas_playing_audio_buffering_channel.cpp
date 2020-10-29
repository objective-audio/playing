//
//  yas_playing_audio_buffering_channel.cpp
//

#include "yas_playing_audio_buffering_channel.h"

#include <cpp_utils/yas_fast_each.h>

#include <thread>

#include "yas_playing_audio_buffering_element.h"

using namespace yas;
using namespace yas::playing;

audio_buffering_channel::audio_buffering_channel(std::vector<audio_buffering_element_protocol_ptr> &&elements)
    : _elements(std::move(elements)) {
}

void audio_buffering_channel::write_all_elements_on_task(path::channel const &ch_path,
                                                         fragment_index_t const top_frag_idx) {
    this->_ch_path = ch_path;

    fragment_index_t element_frag_idx = top_frag_idx;
    for (auto &element : this->_elements) {
        element->force_write_on_task(ch_path, element_frag_idx);
        ++element_frag_idx;

        std::this_thread::yield();
    }
}

bool audio_buffering_channel::write_elements_if_needed_on_task() {
    bool is_written = false;

    for (auto &element : this->_elements) {
        if (element->write_if_needed_on_task(this->_ch_path.value())) {
            is_written = true;
        }
    }

    return is_written;
}

void audio_buffering_channel::advance_on_render(fragment_index_t const prev_frag_idx,
                                                fragment_index_t const new_frag_idx) {
    for (auto const &element : this->_elements) {
        if (element->fragment_index_on_render() == prev_frag_idx) {
            element->advance_on_render(new_frag_idx);
        }
    }
}

void audio_buffering_channel::overwrite_element_on_render(fragment_index_t const frag_idx) {
    for (auto const &element : this->_elements) {
        if (element->fragment_index_on_render() == frag_idx) {
            element->overwrite_on_render();
        }
    }
}

bool audio_buffering_channel::read_into_buffer_on_render(audio::pcm_buffer *out_buffer, frame_index_t const frame) {
    for (auto const &element : this->_elements) {
        if (element->contains_frame_on_render(frame)) {
            return element->read_into_buffer_on_render(out_buffer, frame);
        }
    }

    return false;
}

std::vector<std::shared_ptr<audio_buffering_element_protocol>> const &audio_buffering_channel::elements_for_test()
    const {
    return this->_elements;
}

audio_buffering_channel_ptr audio_buffering_channel::make_shared(
    std::vector<audio_buffering_element_protocol_ptr> &&elements) {
    return audio_buffering_channel_ptr{new audio_buffering_channel{std::move(elements)}};
}
