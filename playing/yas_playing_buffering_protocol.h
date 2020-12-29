//
//  yas_playing_buffering_protocol.h
//

#pragma once

namespace yas::playing {
enum class audio_buffering_setup_state {
    /// 新規生成後の待機状態
    /// render側: 無条件でcreatingにする
    /// task側: 何もしない
    initial,

    /// bufferを作る状態
    /// render側: 何もしない
    /// task側: bufferを作って終わったらrendering_stateをwaitingにし、renderingにする
    creating,

    /// bufferがある状態
    /// render側: 読み込みに使う。フォーマットが合わなければcreatingにする
    /// task側: 何もしない
    rendering,
};

enum class audio_buffering_rendering_state {
    /// 待機。初期状態
    /// render側: 無条件でall_writingにする。
    /// task側: 何もしない
    waiting,

    /// 全てのバッファに書き込む状態
    /// render側: 何もしない。
    /// task側: 全体を書き込み終わったらadvancingにする
    all_writing,

    /// 再生して進む状態。主に個別のバッファを扱う
    /// render側:
    /// seekされたりch_mappingが変更されたらall_writingにする。
    /// 読み込んでバッファの最後まで行ったら個別にwritableにする。
    /// task側:
    /// buffering的には何もしない
    /// 個別のバッファがwritableならファイルから読み込んで、終わったらreadableにする
    advancing,
};

struct buffering_protocol {
    using setup_state_t = audio_buffering_setup_state;
    using rendering_state_t = audio_buffering_rendering_state;

    virtual ~buffering_protocol() = default;

    [[nodiscard]] virtual setup_state_t setup_state() const = 0;
    [[nodiscard]] virtual rendering_state_t rendering_state() const = 0;
    [[nodiscard]] virtual std::size_t element_count() const = 0;
    [[nodiscard]] virtual std::size_t channel_count_on_render() const = 0;
    [[nodiscard]] virtual sample_rate_t fragment_length_on_render() const = 0;

    virtual void set_creating_on_render(double const sample_rate, audio::pcm_format const &,
                                        uint32_t const ch_count) = 0;
    [[nodiscard]] virtual bool needs_create_on_render(double const sample_rate, audio::pcm_format const &,
                                                      uint32_t const ch_count) = 0;

    virtual void create_buffer_on_task() = 0;

    virtual void set_all_writing_on_render(frame_index_t const, std::optional<channel_mapping_ptr> &&ch_mapping) = 0;
    virtual void write_all_elements_on_task() = 0;
    virtual void advance_on_render(fragment_index_t const) = 0;
    [[nodiscard]] virtual bool write_elements_if_needed_on_task() = 0;
    virtual void overwrite_element_on_render(element_address const &) = 0;

    [[nodiscard]] virtual bool read_into_buffer_on_render(audio::pcm_buffer *, channel_index_t const,
                                                          frame_index_t const) = 0;
};
}  // namespace yas::playing
