//
//  yas_playing_audio_buffering_element_protocol.h
//

#pragma once
//
#include <audio/yas_audio_pcm_buffer.h>
#include <playing/yas_playing_path.h>

namespace yas::playing {
enum class audio_buffering_element_state {
    /// 新規作成後の待機状態。初回の書き込み中もここ
    initial,
    /// ファイルからバッファに書き込み中
    writable,
    /// バッファにデータが準備され読み込み中
    readable,
};

struct audio_buffering_element_protocol {
    using state_t = audio_buffering_element_state;

    virtual ~audio_buffering_element_protocol() = default;

    [[nodiscard]] virtual state_t state() const = 0;
    [[nodiscard]] virtual fragment_index_t fragment_index_on_render() const = 0;

    [[nodiscard]] virtual bool write_if_needed_on_task(path::channel const &) = 0;
    virtual void force_write_on_task(path::channel const &, fragment_index_t const) = 0;

    [[nodiscard]] virtual bool contains_frame_on_render(frame_index_t const) = 0;
    [[nodiscard]] virtual bool read_into_buffer_on_render(audio::pcm_buffer *, frame_index_t const) = 0;
    virtual void advance_on_render(fragment_index_t const) = 0;
    virtual void overwrite_on_render() = 0;
};
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::audio_buffering_element_state const &);
}
