//
//  yas_playing_audio_player.cpp
//

#include "yas_playing_audio_player.h"

#include <thread>

#include "yas_playing_audio_buffering.h"
#include "yas_playing_audio_buffering_channel.h"
#include "yas_playing_audio_buffering_element.h"
#include "yas_playing_audio_player_utils.h"
#include "yas_playing_audio_reading.h"
#include "yas_playing_audio_rendering.h"
#include "yas_playing_audio_rendering_info.h"
#include "yas_playing_path.h"
#include "yas_playing_ptr.h"

using namespace yas;
using namespace yas::playing;

audio_player::audio_player(audio_renderable_ptr const &renderable, std::string const &root_path,
                           worker_ptr const &worker, task_priority const &priority,
                           audio_rendering_protocol_ptr const &rendering, audio_reading_protocol_ptr const &reading,
                           audio_buffering_protocol_ptr const &buffering)
    : _renderable(renderable),
      _worker(worker),
      _priority(priority),
      _rendering(rendering),
      _reading(reading),
      _buffering(buffering) {
    if (priority.rendering <= priority.setup) {
        throw std::invalid_argument("invalid priority");
    }

    // setup worker

    worker->add_task(priority.setup,
                     [reading = this->_reading, buffering = this->_buffering, rendering = this->_rendering] {
                         auto result = worker::task_result::unprocessed;

                         if (reading->state() == audio_reading::state_t::creating) {
                             reading->create_buffer_on_task();
                             std::this_thread::yield();
                             result = worker::task_result::processed;
                             std::this_thread::yield();
                         }

                         if (buffering->setup_state() == audio_buffering::setup_state_t::creating) {
                             buffering->create_buffer_on_task();
                             std::this_thread::yield();
                             result = worker::task_result::processed;
                             std::this_thread::yield();
                         }

                         return result;
                     });

    worker->add_task(priority.rendering, [buffering = this->_buffering, rendering = this->_rendering] {
#warning 細かく処理を分ける&他のステートを見て途中でやめる
        switch (buffering->rendering_state()) {
            case audio_buffering::rendering_state_t::waiting:
                return worker::task_result::unprocessed;
            case audio_buffering::rendering_state_t::all_writing:
                buffering->write_all_elements_on_task();
                return worker::task_result::processed;
            case audio_buffering::rendering_state_t::advancing:
                if (buffering->write_elements_if_needed_on_task()) {
                    return worker::task_result::processed;
                } else {
                    return worker::task_result::unprocessed;
                }
        }
    });

    // setup chaining

    this->_pool +=
        this->_ch_mapping->chain()
            .perform([this](auto const &ch_mapping) { this->_rendering->set_ch_mapping_on_main(ch_mapping); })
            .sync();

    this->_pool += this->_is_playing->chain()
                       .perform([this](bool const &is_playing) { this->_update_playing(is_playing); })
                       .sync();

    // setup renderable

    audio_rendering::overwrite_requests_f overwrite_requests_handler =
        [buffering = this->_buffering](audio_rendering::overwrite_requests_t const &requests) {
            for (auto const &request : requests) {
                buffering->overwrite_element_on_render(request);
            }
        };

    this->_renderable->set_rendering_handler(
        [rendering = this->_rendering, reading = this->_reading, buffering = this->_buffering,
         overwrite_requests_handler = std::move(overwrite_requests_handler)](audio::pcm_buffer *const out_buffer) {
            auto const &out_format = out_buffer->format();
            auto const sample_rate = out_format.sample_rate();
            auto const pcm_format = out_format.pcm_format();
            auto const out_length = out_buffer->frame_length();
            auto const out_ch_count = out_format.channel_count();

            if (out_format.is_interleaved()) {
                throw std::invalid_argument("out_buffer is not non-interleaved.");
            }

            // 出力バッファのセットアップ

            switch (reading->state()) {
                case audio_reading::state_t::initial:
                    // 初期状態なのでバッファ生成開始
                    reading->set_creating_on_render(sample_rate, pcm_format, out_length);
                    return;
                case audio_reading::state_t::creating:
                    // task側で生成中
                    return;
                case audio_reading::state_t::rendering:
                    // バッファ生成済み
                    break;
            }

            // 生成済みの出力バッファを作り直すかチェック
            if (reading->needs_create_on_render(sample_rate, pcm_format, out_length)) {
                // バッファを再生成
                reading->set_creating_on_render(sample_rate, pcm_format, out_length);
                return;
            }

            // リングバッファのセットアップ

            switch (buffering->setup_state()) {
                case audio_buffering::setup_state_t::initial:
                    // 初期状態なのでバッファ生成開始
                    buffering->set_creating_on_render(sample_rate, pcm_format, out_ch_count);
                    return;
                case audio_buffering::setup_state_t::creating:
                    // task側で生成中
                    return;
                case audio_buffering::setup_state_t::rendering:
                    // バッファ生成済み
                    break;
            }

            // 生成済みのバッファを作り直すかチェック
            if (buffering->needs_create_on_render(sample_rate, pcm_format, out_ch_count)) {
                // リングバッファの再生成
                buffering->set_creating_on_render(sample_rate, pcm_format, out_ch_count);
                return;
            }

            // リングバッファのレンダリング

            switch (buffering->rendering_state()) {
                case audio_buffering::rendering_state_t::waiting: {
                    // 書き込み待機状態なので全バッファ書き込み開始
                    rendering->reset_overwrite_requests_on_render();
                    auto const seek_frame = rendering->pull_seek_frame_on_render();
                    frame_index_t const frame =
                        seek_frame.has_value() ? seek_frame.value() : rendering->play_frame_on_render();
                    buffering->set_all_writing_on_render(frame, rendering->pull_ch_mapping_on_render());
                }
                    return;
                case audio_buffering::rendering_state_t::all_writing:
                    // task側で書き込み中
                    return;
                case audio_buffering::rendering_state_t::advancing:
                    // 全バッファ書き込み済み
                    break;
            }

            // シークされたかチェック
            if (auto const seek_frame = rendering->pull_seek_frame_on_render(); seek_frame.has_value()) {
                // 全バッファ再書き込み開始
                rendering->reset_overwrite_requests_on_render();
                auto const &frame = seek_frame.value();
                rendering->set_play_frame_on_render(frame);
                buffering->set_all_writing_on_render(frame, rendering->pull_ch_mapping_on_render());
                return;
            }

            // チャンネルマッピングが変更されたかチェック
            if (auto ch_mapping = rendering->pull_ch_mapping_on_render(); ch_mapping.has_value()) {
                // 全バッファ再書き込み開始
                rendering->reset_overwrite_requests_on_render();
                buffering->set_all_writing_on_render(rendering->play_frame_on_render(), std::move(ch_mapping));
                return;
            }

            // リングバッファの要素の上書き
            rendering->perform_overwrite_requests_on_render(overwrite_requests_handler);

            // 再生中でなければ終了
            if (!rendering->is_playing_on_render()) {
                return;
            }

            // 以下レンダリング

            frame_index_t const begin_frame = rendering->play_frame_on_render();
            frame_index_t current_frame = begin_frame;
            frame_index_t const next_frame = current_frame + out_length;
            uint32_t const frag_length = buffering->fragment_length_on_render();

            audio::pcm_buffer *reading_buffer = reading->buffer_on_render();
            if (!reading_buffer) {
                throw std::runtime_error("reading_buffer is null.");
            }

            bool read_failed = false;

            while (current_frame < next_frame) {
                audio_rendering_info const info{current_frame, next_frame, frag_length};
                uint32_t const to_frame = uint32_t(current_frame - begin_frame);

                auto each = make_fast_each(out_format.channel_count());
                while (yas_each_next(each)) {
                    auto const &idx = yas_each_index(each);

                    if (buffering->channel_count_on_render() <= idx) {
                        break;
                    }

                    reading_buffer->clear();
                    reading_buffer->set_frame_length(info.length);

                    if (!buffering->read_into_buffer_on_render(reading_buffer, idx, current_frame)) {
                        read_failed = true;
                        break;
                    }

                    out_buffer->copy_channel_from(
                        *reading_buffer, {.to_channel = idx, .to_begin_frame = to_frame, .length = info.length});
                }

                if (read_failed) {
                    break;
                }

                if (info.next_frag_idx.has_value()) {
                    buffering->advance_on_render(*info.next_frag_idx);
                }

                current_frame += info.length;

                rendering->set_play_frame_on_render(current_frame);
            }
        });

    this->_renderable->set_is_rendering(true);
}

void audio_player::set_ch_mapping(std::vector<int64_t> ch_mapping) {
    this->_ch_mapping->set_value(std::move(ch_mapping));
}

void audio_player::set_playing(bool const is_playing) {
    this->_is_playing->set_value(is_playing);
}

void audio_player::seek(frame_index_t const frame) {
    this->_rendering->seek_on_main(frame);
}

void audio_player::overwrite(channel_index_t const ch_idx, fragment_index_t const frag_idx) {
    this->_rendering->add_overwrite_request_on_main({.channel_index = ch_idx, .fragment_index = frag_idx});
}

std::vector<channel_index_t> const &audio_player::ch_mapping() const {
    return this->_ch_mapping->raw();
}

bool audio_player::is_playing() const {
    return this->_is_playing->raw();
}

frame_index_t audio_player::play_frame() const {
    return this->_rendering->play_frame_on_main();
}

chaining::chain_sync_t<bool> audio_player::is_playing_chain() const {
    return this->_is_playing->chain();
}

player_ptr audio_player::make_shared(audio_renderable_ptr const &renderable, std::string const &root_path,
                                     worker_ptr const &worker, task_priority const &priority,
                                     audio_rendering_protocol_ptr const &rendering,
                                     audio_reading_protocol_ptr const &reading,
                                     audio_buffering_protocol_ptr const &buffering) {
    return player_ptr(new audio_player{renderable, root_path, worker, priority, rendering, reading, buffering});
}

void audio_player::_update_playing(bool const is_playing) {
    this->_rendering->set_is_playing_on_main(is_playing);
}
