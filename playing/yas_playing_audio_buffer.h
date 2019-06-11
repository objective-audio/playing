//
//  yas_playing_audio_buffer.h
//

#pragma once

#include <audio/yas_audio_file.h>
#include <audio/yas_audio_format.h>
#include <audio/yas_audio_pcm_buffer.h>
#include <cpp_utils/yas_result.h>
#include <mutex>
#include <optional>
#include "yas_playing_types.h"

namespace yas::playing {
struct audio_buffer {
    using ptr = std::shared_ptr<audio_buffer>;
    using wptr = std::weak_ptr<audio_buffer>;

    enum class state_kind {
        unloaded,
        loaded,
    };

    struct state {
        using ptr = std::shared_ptr<state>;

        std::optional<fragment_index_t> const frag_idx;
        state_kind const kind;

        state();
        state(std::optional<fragment_index_t> const, state_kind const);

        bool operator==(state const &rhs) const;
    };

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

    struct identifier_t : base {
        struct impl : base::impl {};
        identifier_t() : base(std::make_shared<impl>()) {
        }
    };

    using state_changed_f = std::function<void(uintptr_t const, state::ptr const &)>;

    identifier_t const identifier;

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
    state::ptr _state = std::make_shared<state>();
    state_changed_f _state_changed_handler = nullptr;

    std::recursive_mutex mutable _loading_mutex;
    std::recursive_mutex mutable _state_mutex;

    bool _set_state(fragment_index_t const, state_kind const);
    [[nodiscard]] state::ptr _try_get_state() const;
};

audio_buffer::ptr make_audio_buffer(audio::pcm_buffer &&, audio_buffer::state_changed_f);
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::audio_buffer::state_kind const &state);
std::string to_string(playing::audio_buffer::load_error const &error);
std::string to_string(playing::audio_buffer::read_error const &error);
}  // namespace yas

std::ostream &operator<<(std::ostream &, yas::playing::audio_buffer::state_kind const &);
std::ostream &operator<<(std::ostream &, yas::playing::audio_buffer::load_error const &);
std::ostream &operator<<(std::ostream &, yas::playing::audio_buffer::read_error const &);
