//
//  yas_playing_player.cpp
//

#include "yas_playing_player.h"

#include <playing/yas_playing_channel_mapping.h>

#include <thread>

#include "yas_playing_buffering_channel.h"
#include "yas_playing_buffering_element.h"
#include "yas_playing_buffering_resource.h"
#include "yas_playing_path.h"
#include "yas_playing_player_resource.h"
#include "yas_playing_player_utils.h"
#include "yas_playing_ptr.h"
#include "yas_playing_reading_resource.h"

using namespace yas;
using namespace yas::playing;

player::player(std::string const &root_path, renderable_ptr const &renderer, workable_ptr const &worker,
               task_priority_t const &priority, player_resource_protocol_ptr const &resource)
    : _renderer(renderer),
      _worker(worker),
      _priority(priority),
      _resource(resource),
      _ch_mapping(chaining::value::holder<channel_mapping_ptr>::make_shared(channel_mapping::make_shared())) {
    if (priority.rendering <= priority.setup) {
        throw std::invalid_argument("invalid priority");
    }

    // setup worker

    worker->add_task(priority.setup, [resource = this->_resource] {
        auto const &reading = resource->reading();
        auto const &buffering = resource->buffering();

        auto result = worker::task_result::unprocessed;

        if (reading->state() == reading_resource::state_t::creating) {
            reading->create_buffer_on_task();
            std::this_thread::yield();
            result = worker::task_result::processed;
            std::this_thread::yield();
        }

        if (buffering->setup_state() == buffering_resource::setup_state_t::creating) {
            buffering->create_buffer_on_task();
            std::this_thread::yield();
            result = worker::task_result::processed;
            std::this_thread::yield();
        }

        return result;
    });

    worker->add_task(priority.rendering, [buffering = this->_resource->buffering()] {
#warning 細かく処理を分ける&他のステートを見て途中でやめる
        switch (buffering->rendering_state()) {
            case buffering_resource::rendering_state_t::waiting:
                return worker::task_result::unprocessed;
            case buffering_resource::rendering_state_t::all_writing:
                buffering->write_all_elements_on_task();
                return worker::task_result::processed;
            case buffering_resource::rendering_state_t::advancing:
                if (buffering->write_elements_if_needed_on_task()) {
                    return worker::task_result::processed;
                } else {
                    return worker::task_result::unprocessed;
                }
        }
    });

    // setup chaining

    this->_ch_mapping->chain()
        .perform([this](auto const &ch_mapping) { this->_resource->set_channel_mapping_on_main(ch_mapping); })
        .sync()
        ->add_to(this->_pool);

    this->_is_playing->chain()
        .perform([this](bool const &is_playing) { this->_resource->set_playing_on_main(is_playing); })
        .sync()
        ->add_to(this->_pool);

    // setup renderable

    player_resource::overwrite_requests_f overwrite_requests_handler =
        [buffering = this->_resource->buffering()](player_resource::overwrite_requests_t const &requests) {
            for (auto const &request : requests) {
                buffering->overwrite_element_on_render(request);
            }
        };

    this->_renderer->set_rendering_handler([resource = this->_resource,
                                            overwrite_requests_handler = std::move(overwrite_requests_handler)](
                                               audio::pcm_buffer *const out_buffer) {
        auto const &reading = resource->reading();
        auto const &buffering = resource->buffering();

        auto const &out_format = out_buffer->format();
        auto const sample_rate = static_cast<sample_rate_t>(std::round(out_format.sample_rate()));
        auto const pcm_format = out_format.pcm_format();
        auto const out_length = out_buffer->frame_length();
        auto const out_ch_count = out_format.channel_count();

        if (out_format.is_interleaved()) {
            throw std::invalid_argument("out_buffer is not non-interleaved.");
        }

        // reading_resourceのセットアップ

        switch (reading->state()) {
            case reading_resource::state_t::initial:
                // 初期状態なのでバッファ生成開始
                reading->set_creating_on_render(sample_rate, pcm_format, out_length);
                return;
            case reading_resource::state_t::creating:
                // task側で生成中
                return;
            case reading_resource::state_t::rendering:
                // バッファ生成済み
                break;
        }

        // 生成済みのreadingバッファを作り直すかチェック
        if (reading->needs_create_on_render(sample_rate, pcm_format, out_length)) {
            // バッファを再生成
            reading->set_creating_on_render(sample_rate, pcm_format, out_length);
            return;
        }

        // buffering_resourceのセットアップ

        switch (buffering->setup_state()) {
            case buffering_resource::setup_state_t::initial:
                // 初期状態なのでバッファ生成開始
                buffering->set_creating_on_render(sample_rate, pcm_format, out_ch_count);
                return;
            case buffering_resource::setup_state_t::creating:
                // task側で生成中
                return;
            case buffering_resource::setup_state_t::rendering:
                // バッファ生成済み
                break;
        }

        // 生成済みのbufferingバッファを作り直すかチェック
        if (buffering->needs_create_on_render(sample_rate, pcm_format, out_ch_count)) {
            // バッファの再生成
            buffering->set_creating_on_render(sample_rate, pcm_format, out_ch_count);
            return;
        }

        // bufferingバッファへの書き込み

        switch (buffering->rendering_state()) {
            case buffering_resource::rendering_state_t::waiting: {
                // 書き込み待機状態なので全バッファ書き込み開始
                resource->reset_overwrite_requests_on_render();
                auto const seek_frame = resource->pull_seek_frame_on_render();
                frame_index_t const frame = seek_frame.has_value() ? seek_frame.value() : resource->current_frame();
                buffering->set_all_writing_on_render(frame, resource->pull_channel_mapping_on_render(),
                                                     resource->pull_identifier_on_render());
            }
                return;
            case buffering_resource::rendering_state_t::all_writing:
                // task側で書き込み中
                return;
            case buffering_resource::rendering_state_t::advancing:
                // 全バッファ書き込み済み
                break;
        }

        // シークされたかチェック
        if (auto const seek_frame = resource->pull_seek_frame_on_render(); seek_frame.has_value()) {
            // 全バッファ再書き込み開始
            resource->reset_overwrite_requests_on_render();
            auto const &frame = seek_frame.value();
            resource->set_current_frame_on_render(frame);
            buffering->set_all_writing_on_render(frame, resource->pull_channel_mapping_on_render(),
                                                 resource->pull_identifier_on_render());
            return;
        }

        // チャンネルマッピングかIdentifierが変更されたかチェック
        auto ch_mapping = resource->pull_channel_mapping_on_render();
        auto identifier = resource->pull_identifier_on_render();
        if (ch_mapping.has_value() || identifier.has_value()) {
            // 全バッファ再書き込み開始
            resource->reset_overwrite_requests_on_render();
            buffering->set_all_writing_on_render(resource->current_frame(), std::move(ch_mapping),
                                                 std::move(identifier));
            return;
        }

        // bufferingの要素の上書き
        resource->perform_overwrite_requests_on_render(overwrite_requests_handler);

        // 再生中でなければ終了
        if (!resource->is_playing_on_render()) {
            return;
        }

        // 以下レンダリング

        audio::pcm_buffer *reading_buffer = reading->buffer_on_render();
        if (!reading_buffer) {
            return;
        }

        frame_index_t const begin_frame = resource->current_frame();
        frame_index_t current_frame = begin_frame;
        frame_index_t const next_frame = current_frame + out_length;
        uint32_t const frag_length = buffering->fragment_length_on_render();

        while (current_frame < next_frame) {
            auto const proc_length = player_utils::process_length(current_frame, next_frame, frag_length);
            uint32_t const to_frame = uint32_t(current_frame - begin_frame);

            bool read_failed = false;

            auto each = make_fast_each(out_format.channel_count());
            while (yas_each_next(each)) {
                auto const &idx = yas_each_index(each);

                if (buffering->channel_count_on_render() <= idx) {
                    break;
                }

                reading_buffer->clear();
                reading_buffer->set_frame_length(proc_length);

                if (!buffering->read_into_buffer_on_render(reading_buffer, idx, current_frame)) {
                    read_failed = true;
                    break;
                }

                out_buffer->copy_channel_from(*reading_buffer,
                                              {.to_channel = idx, .to_begin_frame = to_frame, .length = proc_length});
            }

            if (read_failed) {
                break;
            }

            if (auto const frag_idx = player_utils::advancing_fragment_index(current_frame, proc_length, frag_length);
                frag_idx.has_value()) {
                buffering->advance_on_render(frag_idx.value());
            }

            current_frame += proc_length;

            resource->set_current_frame_on_render(current_frame);
        }
    });

    this->_renderer->set_is_rendering(true);
}

void player::set_channel_mapping(channel_mapping_ptr const &ch_mapping) {
    this->_ch_mapping->set_value(ch_mapping);
}

void player::set_playing(bool const is_playing) {
    this->_is_playing->set_value(is_playing);
}

void player::seek(frame_index_t const frame) {
    this->_resource->seek_on_main(frame);
}

void player::overwrite(std::optional<channel_index_t> const file_ch_idx, fragment_range const frag_range) {
    this->_resource->add_overwrite_request_on_main({.file_channel_index = file_ch_idx, .fragment_range = frag_range});
}

channel_mapping_ptr const &player::channel_mapping() const {
    return this->_ch_mapping->raw();
}

bool player::is_playing() const {
    return this->_is_playing->raw();
}

frame_index_t player::current_frame() const {
    return this->_resource->current_frame();
}

chaining::chain_sync_t<bool> player::is_playing_chain() const {
    return this->_is_playing->chain();
}

player_ptr player::make_shared(std::string const &root_path, renderable_ptr const &renderer, workable_ptr const &worker,
                               task_priority_t const &priority, player_resource_protocol_ptr const &resource) {
    return player_ptr(new player{root_path, renderer, worker, priority, resource});
}
