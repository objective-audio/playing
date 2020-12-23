//
//  yas_playing_audio_buffering.cpp
//

#include "yas_playing_audio_buffering.h"

#include <audio/yas_audio_pcm_buffer.h>
#include <cpp_utils/yas_fast_each.h>
#include <cpp_utils/yas_file_manager.h>
#include <cpp_utils/yas_result.h>

#include <thread>

#include "yas_playing_audio_buffering_channel.h"
#include "yas_playing_audio_buffering_element.h"
#include "yas_playing_audio_player_utils.h"
#include "yas_playing_signal_file.h"
#include "yas_playing_signal_file_info.h"

using namespace yas;
using namespace yas::playing;

audio_buffering::audio_buffering(std::size_t const element_count, std::string const &root_path,
                                 make_channel_f &&make_channel_handler)
    : _element_count(element_count), _root_path(root_path), _make_channel_handler(make_channel_handler) {
}

std::size_t audio_buffering::element_count() const {
    return this->_element_count;
}

audio_buffering::setup_state_t audio_buffering::setup_state() const {
    return this->_setup_state.load();
}

audio_buffering::rendering_state_t audio_buffering::rendering_state() const {
    return this->_rendering_state.load();
}

std::size_t audio_buffering::channel_count_on_render() const {
    return this->_ch_count;
}

sample_rate_t audio_buffering::fragment_length_on_render() const {
    return this->_frag_length;
}

void audio_buffering::set_creating_on_render(double const sample_rate, audio::pcm_format const &pcm_format,
                                             uint32_t const ch_count) {
    if (this->_setup_state.load() == setup_state_t::creating) {
        throw std::runtime_error("state is already creating.");
    }

    this->_sample_rate = sample_rate;
    this->_frag_length = round(sample_rate);
    this->_pcm_format = pcm_format;
    this->_ch_count = ch_count;
    this->_setup_state.store(setup_state_t::creating);
}

bool audio_buffering::needs_create_on_render(double const sample_rate, audio::pcm_format const &pcm_format,
                                             uint32_t const ch_count) {
    if (this->_setup_state.load() != setup_state_t::rendering) {
        throw std::runtime_error("state is not rendering.");
    }

    if (this->_sample_rate != sample_rate) {
        return true;
    }

    if (this->_pcm_format != pcm_format) {
        return true;
    }

    if (this->_ch_count != ch_count) {
        return true;
    }

    return false;
}

void audio_buffering::create_buffer_on_task() {
    if (this->_setup_state.load() != setup_state_t::creating) {
        throw std::runtime_error("state is not creating.");
    }

    this->_format = std::nullopt;
    this->_tl_path = std::nullopt;
    this->_channels.clear();

    std::this_thread::yield();

    if (this->_sample_rate == 0) {
        throw std::runtime_error("sample_rate is zero.");
    }

    if (this->_frag_length == 0) {
        throw std::runtime_error("frag_length is zero.");
    }

    if (this->_ch_count == 0) {
        throw std::runtime_error("ch_count is zero.");
    }

    audio::format const format{
        {.sample_rate = this->_sample_rate, .pcm_format = this->_pcm_format, .interleaved = false, .channel_count = 1}};

    this->_format = format;

    std::this_thread::yield();

    auto ch_each = make_fast_each(this->_ch_count);
    while (yas_each_next(ch_each)) {
        this->_channels.emplace_back(this->_make_channel_handler(this->_element_count, format, this->_sample_rate));

        std::this_thread::yield();
    }

#warning todo identifierが0以外も対応したい
    this->_tl_path = path::timeline{.root_path = this->_root_path,
                                    .identifier = "0",
                                    .sample_rate = static_cast<sample_rate_t>(this->_sample_rate)};

    this->_rendering_state.store(rendering_state_t::waiting);

    std::this_thread::yield();

    this->_setup_state.store(setup_state_t::rendering);

    std::this_thread::yield();
}

void audio_buffering::set_all_writing_on_render(frame_index_t const frame,
                                                std::optional<std::vector<channel_index_t>> &&ch_mapping) {
    if (this->_rendering_state.load() == rendering_state_t::all_writing) {
        throw std::runtime_error("state is already all_writing.");
    }

    this->_all_writing_frame = frame;
    if (ch_mapping.has_value()) {
        this->_ch_mapping = ch_mapping.value();
    }
    this->_rendering_state.store(rendering_state_t::all_writing);
}

void audio_buffering::write_all_elements_on_task() {
    if (this->_rendering_state.load() != rendering_state_t::all_writing) {
        throw std::runtime_error("state is not all_writing.");
    }

    auto const top_frag_idx = audio_player_utils::top_fragment_idx(this->_frag_length, this->_all_writing_frame);
    if (!top_frag_idx.has_value()) {
        throw std::runtime_error("sample_rate is empty.");
    }

    channel_index_t ch_idx = 0;
    for (auto const &channel : this->_channels) {
        path::channel const ch_path{*this->_tl_path, this->_mapped_ch_idx_on_task(ch_idx)};

        channel->write_all_elements_on_task(ch_path, top_frag_idx.value());

        ++ch_idx;

        std::this_thread::yield();
    }

    std::this_thread::yield();

    this->_rendering_state.store(rendering_state_t::advancing);

    std::this_thread::yield();
}

void audio_buffering::advance_on_render(fragment_index_t const frag_idx) {
    if (this->_rendering_state.load() != rendering_state_t::advancing) {
        throw std::runtime_error("state is not advancing.");
    }

    auto const prev_frag_idx = frag_idx - this->_element_count;
    for (auto const &channel : this->_channels) {
        channel->advance_on_render(prev_frag_idx, frag_idx);
    }
}

bool audio_buffering::write_elements_if_needed_on_task() {
    if (this->_rendering_state.load() != rendering_state_t::advancing) {
        throw std::runtime_error("state is not advancing.");
    }

    bool is_loaded = false;

    for (auto const &channel : this->_channels) {
        if (channel->write_elements_if_needed_on_task()) {
            is_loaded = true;
        }

        std::this_thread::yield();
    }

    return is_loaded;
}

void audio_buffering::overwrite_element_on_render(element_address const &index) {
    if (this->_rendering_state.load() != rendering_state_t::advancing) {
        throw std::runtime_error("state is not advancing.");
    }

    if (auto const idx = this->_unmapped_ch_idx_on_task(index.channel_index); idx.has_value()) {
        this->_channels.at(idx.value())->overwrite_element_on_render(index.fragment_index);
    }
}

bool audio_buffering::read_into_buffer_on_render(audio::pcm_buffer *out_buffer, channel_index_t const ch_idx,
                                                 frame_index_t const frame) {
    if (this->_channels.size() <= ch_idx) {
        return false;
    }

    return this->_channels.at(ch_idx)->read_into_buffer_on_render(out_buffer, frame);
}

audio_buffering_ptr audio_buffering::make_shared(std::size_t const element_count, std::string const &root_path,

                                                 make_channel_f &&make_channel_handler) {
    return audio_buffering_ptr{new audio_buffering{element_count, root_path, std::move(make_channel_handler)}};
}

frame_index_t audio_buffering::all_writing_frame_for_test() const {
    return this->_all_writing_frame;
}

std::vector<channel_index_t> const &audio_buffering::ch_mapping_for_test() const {
    return this->_ch_mapping;
}

channel_index_t audio_buffering::_mapped_ch_idx_on_task(channel_index_t const ch_idx) const {
    if (ch_idx < this->_ch_mapping.size()) {
        return this->_ch_mapping.at(ch_idx);
    } else {
        return ch_idx;
    }
}

std::optional<channel_index_t> audio_buffering::_unmapped_ch_idx_on_task(channel_index_t const ch_idx) const {
    auto each = make_fast_each(this->_ch_mapping.size());
    while (yas_each_next(each)) {
        auto const &idx = yas_each_index(each);
        if (this->_ch_mapping.at(idx) == ch_idx) {
            return idx;
        }
    }

    if (this->_ch_mapping.size() <= ch_idx && ch_idx < this->_channels.size()) {
        return ch_idx;
    }

    return std::nullopt;
}