//
//  yas_playing_audio_buffer.cpp
//

#include "yas_playing_audio_buffer.h"
#include <thread>

using namespace yas;
using namespace yas::playing;

#pragma mark - audio_buffer::state

audio_buffer::state::state() : frag_idx(std::nullopt), kind(state_kind::unloaded) {
}

audio_buffer::state::state(std::optional<fragment_index_t> const frag_idx, state_kind const kind)
    : frag_idx(frag_idx), kind(kind) {
}

bool audio_buffer::state::operator==(state const &rhs) const {
    if (this->frag_idx && rhs.frag_idx) {
        return *this->frag_idx == *rhs.frag_idx && this->kind == rhs.kind;
    } else if (!this->frag_idx && !rhs.frag_idx) {
        return this->kind == rhs.kind;
    } else {
        return false;
    }
}

#pragma mark - audio_buffer

audio_buffer::audio_buffer(audio::pcm_buffer &&buffer, state_changed_f &&handler)
    : _buffer(std::move(buffer)), _state_changed_handler(std::move(handler)) {
}

std::optional<fragment_index_t> audio_buffer::fragment_idx() const {
    return this->_state->frag_idx;
}

std::optional<frame_index_t> audio_buffer::begin_frame() const {
    if (auto const frag_idx = this->_state->frag_idx) {
        return *frag_idx * static_cast<frame_index_t>(this->_buffer.frame_length());
    } else {
        return std::nullopt;
    }
}

audio::format const &audio_buffer::format() const {
    return this->_buffer.format();
}

void audio_buffer::prepare_loading(fragment_index_t const frag_idx) {
    auto lock = std::unique_lock<std::recursive_mutex>(this->_loading_mutex, std::try_to_lock);

    this->_set_state(frag_idx, state_kind::unloaded);
}

audio_buffer::load_result_t audio_buffer::load(fragment_index_t const frag_idx, load_f const &handler) {
    std::lock_guard<std::recursive_mutex> lock(this->_loading_mutex);

    state::ptr const prev_state = this->_try_get_state();
    if (!prev_state) {
        return load_result_t{load_error::locked};
    }

    if (!prev_state->frag_idx) {
        return load_result_t{load_error::fragment_idx_is_null};
    }

    if (*prev_state->frag_idx != frag_idx) {
        return load_result_t{load_error::invalid_fragment_idx};
    }

    if (handler(this->_buffer, frag_idx)) {
        if (this->_set_state(frag_idx, state_kind::loaded)) {
            return load_result_t{nullptr};
        } else {
            return load_result_t{load_error::set_state_failed};
        }
    } else {
        return load_result_t{load_error::write_buffer_failed};
    }
}

audio_buffer::read_result_t audio_buffer::read_into_buffer(audio::pcm_buffer &to_buffer,
                                                           frame_index_t const play_frame) const {
    auto lock = std::unique_lock<std::recursive_mutex>(this->_loading_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return read_result_t{read_error::locked};
    }

    auto const state = this->_try_get_state();
    if (!state) {
        return read_result_t{read_error::locked};
    }

    if (state->kind != state_kind::loaded) {
        return read_result_t{read_error::unloaded};
    }

    auto begin_frame_opt = this->begin_frame();
    if (!begin_frame_opt.has_value()) {
        return read_result_t{read_error::begin_frame_is_null};
    }

    frame_index_t const begin_frame = *begin_frame_opt;
    frame_index_t const from_frame = play_frame - begin_frame;

    if (from_frame < 0 || this->_buffer.frame_length() <= from_frame) {
        return read_result_t{read_error::out_of_range_play_frame};
    }

    if (begin_frame + this->_buffer.frame_length() < play_frame + to_buffer.frame_length()) {
        return read_result_t{read_error::out_of_range_length};
    }

    if (auto const result = to_buffer.copy_from(this->_buffer, {.from_begin_frame = static_cast<uint32_t>(from_frame),
                                                                .length = to_buffer.frame_length()})) {
        return read_result_t{nullptr};
    } else {
        return read_result_t{read_error::copy_failed};
    }
}

bool audio_buffer::_set_state(fragment_index_t const frag_idx, state_kind const kind) {
    std::lock_guard<std::recursive_mutex> lock(this->_state_mutex);

    if (kind == state_kind::loaded && frag_idx != this->_state->frag_idx) {
        return false;
    }

    this->_state = std::make_shared<state>(frag_idx, kind);

    std::thread thread{[handler = this->_state_changed_handler, identifier = this->identifier, state = this->_state] {
        handler(identifier, state);
    }};

    thread.detach();

    return true;
}

audio_buffer::state::ptr audio_buffer::_try_get_state() const {
    if (auto lock = std::unique_lock<std::recursive_mutex>(this->_state_mutex, std::try_to_lock); lock.owns_lock()) {
        return this->_state;
    } else {
        return nullptr;
    }
}

#pragma mark -

struct audio_buffer_factory : audio_buffer {
    audio_buffer_factory(audio::pcm_buffer &&buffer, state_changed_f &&handler)
        : audio_buffer(std::move(buffer), std::move(handler)) {
    }
};

audio_buffer::ptr playing::make_audio_buffer(audio::pcm_buffer &&buffer, audio_buffer::state_changed_f handler) {
    return std::make_shared<audio_buffer_factory>(std::move(buffer), std::move(handler));
}

#pragma mark -

std::string yas::to_string(playing::audio_buffer::state_kind const &state) {
    switch (state) {
        case audio_buffer::state_kind::loaded:
            return "loaded";
        case audio_buffer::state_kind::unloaded:
            return "unloaded";
    }
}

std::string yas::to_string(audio_buffer::load_error const &error) {
    switch (error) {
        case audio_buffer::load_error::locked:
            return "locked";
        case audio_buffer::load_error::fragment_idx_is_null:
            return "file_idx_is_null";
        case audio_buffer::load_error::invalid_fragment_idx:
            return "invalid_file_idx";
        case audio_buffer::load_error::write_buffer_failed:
            return "write_buffer_failed";
        case audio_buffer::load_error::set_state_failed:
            return "set_state_failed";
    }
}

std::string yas::to_string(audio_buffer::read_error const &error) {
    switch (error) {
        case audio_buffer::read_error::locked:
            return "locked";
        case audio_buffer::read_error::unloaded:
            return "unloaded";
        case audio_buffer::read_error::begin_frame_is_null:
            return "begin_frame_not_found";
        case audio_buffer::read_error::out_of_range_play_frame:
            return "out_of_range_play_frame";
        case audio_buffer::read_error::out_of_range_length:
            return "out_of_range_length";
        case audio_buffer::read_error::copy_failed:
            return "copy_failed";
    }
}

std::ostream &operator<<(std::ostream &stream, yas::playing::audio_buffer::state_kind const &value) {
    stream << to_string(value);
    return stream;
}

std::ostream &operator<<(std::ostream &stream, yas::playing::audio_buffer::load_error const &value) {
    stream << to_string(value);
    return stream;
}

std::ostream &operator<<(std::ostream &stream, yas::playing::audio_buffer::read_error const &value) {
    stream << to_string(value);
    return stream;
}
