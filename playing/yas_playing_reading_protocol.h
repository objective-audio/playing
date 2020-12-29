//
//  yas_playing_reading_protocol.h
//

#pragma once

namespace yas::playing {
enum class audio_reading_state {
    /// 起動した状態
    /// render側: creatingにする
    /// task側: 何もしない
    initial,

    /// バッファを作る状態
    /// render側: 何もしない
    /// task側: バッファを作る
    creating,

    /// bufferがある状態
    /// render側: レンダリングに使う。フォーマットが合わなければcreatingにする。
    /// task側: 何もしない
    rendering,
};

struct reading_protocol {
    using state_t = audio_reading_state;

    virtual ~reading_protocol() = default;

    [[nodiscard]] virtual state_t state() const = 0;
    [[nodiscard]] virtual audio::pcm_buffer *buffer_on_render() = 0;

    [[nodiscard]] virtual bool needs_create_on_render(double const sample_rate, audio::pcm_format const,
                                                      uint32_t const length) const = 0;
    virtual void set_creating_on_render(double const sample_rate, audio::pcm_format const, uint32_t const length) = 0;
    virtual void create_buffer_on_task() = 0;
};
}  // namespace yas::playing
