//
//  yas_playing_audio_buffer.h
//

#pragma once

#include <audio/yas_audio_file.h>
#include <audio/yas_audio_format.h>
#include <audio/yas_audio_pcm_buffer.h>
#include <cpp_utils/yas_result.h>
#include <cpp_utils/yas_task_protocol.h>

#include <mutex>
#include <optional>

#include "yas_playing_loading_state.h"
#include "yas_playing_ptr.h"
#include "yas_playing_types.h"

namespace yas::playing {
struct cancel_id : task_cancel_id {
    bool is_equal(std::shared_ptr<task_cancel_id> const &rhs) const override {
        if (auto casted_rhs = std::dynamic_pointer_cast<cancel_id>(rhs)) {
            return this->identifier() == casted_rhs->identifier();
        }
        return false;
    }

    uintptr_t identifier() const {
        return reinterpret_cast<uintptr_t>(this);
    }

    static cancel_id_ptr make_shared() {
        return cancel_id_ptr(new cancel_id{});
    }

   private:
    cancel_id() {
    }

    cancel_id(cancel_id const &) = delete;
    cancel_id(cancel_id &&) = delete;
    cancel_id &operator=(cancel_id const &) = delete;
    cancel_id &operator=(cancel_id &&) = delete;
};

struct audio_buffer {
    using ptr = std::shared_ptr<audio_buffer>;
    using wptr = std::weak_ptr<audio_buffer>;

    enum class load_error {
        locked,
        fragment_idx_is_null,
        invalid_fragment_idx,
        write_buffer_failed,
        set_state_failed,
    };

    enum class read_error {
        locked,
        unloaded,
        begin_frame_is_null,
        out_of_range_play_frame,
        out_of_range_length,
        copy_failed,
    };

    using load_result_t = result<std::nullptr_t, load_error>;
    using read_result_t = result<std::nullptr_t, read_error>;

    using load_f = std::function<bool(audio::pcm_buffer &buffer, fragment_index_t const frag_idx)>;

    using state_changed_f = std::function<void(uintptr_t const, loading_state::ptr const)>;

    cancel_id_ptr const identifier = cancel_id::make_shared();

    [[nodiscard]] std::optional<fragment_index_t> fragment_idx() const;
    [[nodiscard]] std::optional<frame_index_t> begin_frame() const;
    [[nodiscard]] audio::format const &format() const;

    void prepare_loading(fragment_index_t const frag_idx);
    load_result_t load(fragment_index_t const frag_idx, load_f const &);
    read_result_t read_into_buffer(audio::pcm_buffer &to_buffer, frame_index_t const play_frame) const;

   protected:
    audio_buffer(audio::pcm_buffer &&, state_changed_f &&);

   private:
    audio::pcm_buffer _buffer;
    loading_state::ptr _state = std::make_shared<loading_state>();
    state_changed_f _state_changed_handler = nullptr;

    std::recursive_mutex mutable _loading_mutex;
    std::recursive_mutex mutable _state_mutex;

    bool _set_state(fragment_index_t const, loading_kind const);
    [[nodiscard]] loading_state::ptr _try_get_state() const;
};

audio_buffer::ptr make_audio_buffer(audio::pcm_buffer &&, audio_buffer::state_changed_f);
}  // namespace yas::playing

namespace yas {

std::string to_string(playing::audio_buffer::load_error const &error);
std::string to_string(playing::audio_buffer::read_error const &error);
}  // namespace yas

std::ostream &operator<<(std::ostream &, yas::playing::audio_buffer::load_error const &);
std::ostream &operator<<(std::ostream &, yas::playing::audio_buffer::read_error const &);
