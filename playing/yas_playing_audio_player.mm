//
//  yas_playing_audio_player.mm
//

#include "yas_playing_audio_player.h"
#include <audio/yas_audio_ptr.h>
#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_fast_each.h>
#include <cpp_utils/yas_file_manager.h>
#include <cpp_utils/yas_thread.h>
#include <atomic>
#include <fstream>
#include "yas_playing_audio_circular_buffer.h"
#include "yas_playing_audio_utils.h"
#include "yas_playing_math.h"
#include "yas_playing_path.h"
#include "yas_playing_signal_file.h"
#include "yas_playing_signal_file_info.h"
#include "yas_playing_timeline_utils.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::audio_player_utils {
proc::sample_rate_t fragment_length(audio::format const &format) {
    return static_cast<proc::sample_rate_t>(format.sample_rate());
}

std::optional<fragment_index_t> top_fragment_idx(proc::sample_rate_t const frag_length,
                                                 frame_index_t const play_frame) {
    if (frag_length > 0) {
        return math::floor_int(play_frame, frag_length) / frag_length;
    } else {
        return std::nullopt;
    }
}
}

namespace yas::playing {
struct audio_player_rendering {
    struct seeking {
        using ptr = std::shared_ptr<seeking>;

        frame_index_t const play_frame;
        bool const is_seeking;

        seeking(frame_index_t const play_frame, bool const is_seeking)
            : play_frame(play_frame), is_seeking(is_seeking) {
        }
    };

    std::atomic<bool> is_playing;
    audio::format const format;
    std::vector<audio_circular_buffer_ptr> const circular_buffers;

    audio_player_rendering(bool const is_playing, frame_index_t const play_frame, audio::format const &format,
                           std::vector<audio_circular_buffer_ptr> &&circular_buffers)
        : is_playing(is_playing),
          _seeking(std::make_shared<seeking>(play_frame, true)),
          format(format),
          circular_buffers(std::move(circular_buffers)) {
    }

    proc::sample_rate_t fragment_length() const {
        return audio_player_utils::fragment_length(this->format);
    }

    std::optional<fragment_index_t> top_fragment_idx() const {
        return audio_player_utils::top_fragment_idx(this->fragment_length(), this->play_frame());
    }

    void seek(frame_index_t const play_frame) {
        assert(thread::is_main());

        this->_update_seeking_on_main(play_frame);

        if (auto const top_frag_idx = this->top_fragment_idx()) {
            for (auto &buffer : this->circular_buffers) {
                buffer->reload_all_buffers(*top_frag_idx);
            }
        }
    }

    frame_index_t play_frame() const {
        return this->_seeking->play_frame;
    }

    bool is_seeking() const {
        return this->_seeking->is_seeking;
    }

    audio::pcm_buffer_ptr const &get_or_create_read_buffer(audio::format const &format, uint32_t const length) {
        assert(!thread::is_main());

        if (this->_read_buffer) {
            if (this->_read_buffer->get()->format() != format) {
                this->_read_buffer = std::nullopt;
            } else if (this->_read_buffer->get()->frame_capacity() < length) {
                this->_read_buffer = std::nullopt;
            }
        }

        if (!this->_read_buffer) {
            this->_read_buffer = std::make_shared<audio::pcm_buffer>(format, length);
        }

        return *this->_read_buffer;
    }

    void update_seeking_after_rendering(frame_index_t const next_frame, frame_index_t const prev_frame) {
        assert(!thread::is_main());

        if (auto lock = std::unique_lock<std::recursive_mutex>(this->_seeking_mutex, std::try_to_lock)) {
            if (this->_seeking->play_frame == prev_frame) {
                this->_seeking = std::make_shared<seeking>(next_frame, false);
            }
        }
    }

   private:
    std::optional<audio::pcm_buffer_ptr> _read_buffer{std::nullopt};
    seeking::ptr _seeking;
    std::recursive_mutex mutable _seeking_mutex;

    void _update_seeking_on_main(frame_index_t const play_frame) {
        std::lock_guard<std::recursive_mutex> lock(this->_seeking_mutex);
        this->_seeking = std::make_shared<seeking>(play_frame, true);
    }
};
}

audio_player::audio_player(audio_renderable_ptr const &renderable, std::string const &root_path,
                           std::shared_ptr<task_queue> const &queue, task_priority_t const priority)
    : _root_path(root_path), _renderable(renderable), _queue(queue), _priority(priority) {
}

void audio_player::set_ch_mapping(std::vector<channel_index_t> ch_mapping) {
    this->_ch_mapping->set_value(std::move(ch_mapping));
}

void audio_player::set_playing(bool const is_playing) {
    this->_is_playing->set_value(is_playing);
}

void audio_player::seek(frame_index_t const play_frame) {
    this->_last_play_frame = play_frame;

    if (auto rendering = this->_rendering) {
        rendering->seek(play_frame);
    }
}

void audio_player::reload(channel_index_t const ch_idx, fragment_index_t const frag_idx) {
    auto rendering = this->_rendering;
    if (!rendering) {
        return;
    }

    if (rendering->circular_buffers.size() <= ch_idx) {
        return;
    }

    rendering->circular_buffers.at(ch_idx)->reload_if_needed(frag_idx);
}

std::string const &audio_player::root_path() const {
    return this->_root_path;
}

std::vector<channel_index_t> const &audio_player::ch_mapping() const {
    return this->_ch_mapping->raw();
}

bool audio_player::is_playing() const {
    return this->_is_playing->raw();
}

frame_index_t audio_player::play_frame() const {
    if (auto rendering = this->_rendering) {
        return rendering->play_frame();
    } else {
        return this->_last_play_frame;
    }
}

chaining::chain_sync_t<bool> audio_player::is_playing_chain() const {
    return this->_is_playing->chain();
}

state_map_vector_holder_t::chain_t audio_player::state_chain() const {
    return this->_state_holder->chain();
}

void audio_player::_prepare(audio_player_ptr const &player) {
    this->_setup_chaining(player);
    this->_setup_rendering_handler(player);
}

void audio_player::_setup_chaining(audio_player_ptr const &player) {
    auto weak_player = to_weak(player);

    this->_update_rendering_receiver = chaining::perform_receiver<>::make_shared([weak_player](auto const &) {
        if (auto player = weak_player.lock()) {
            player->_update_rendering();
        }
    });

    this->_pool += this->_ch_mapping->chain().send_null_to(this->_update_rendering_receiver).sync();

    this->_pool += this->_renderable->chain_sample_rate()
                       .combine(this->_renderable->chain_pcm_format())
                       .to([](std::pair<double, audio::pcm_format> const &pair) {
                           if (pair.first > 0.0) {
                               return std::make_optional(audio::format{audio::format::args{.sample_rate = pair.first,
                                                                                           .channel_count = 1,
                                                                                           .pcm_format = pair.second,
                                                                                           .interleaved = false}});
                           } else {
                               return std::optional<audio::format>{std::nullopt};
                           }
                       })
                       .send_to(this->_format)
                       .sync();

    this->_pool += this->_renderable->chain_channel_count().send_to(this->_ch_count).sync();

    this->_pool += this->_format->chain()
                       .combine(this->_ch_count->chain())
                       .to_null()
                       .send_to(this->_update_rendering_receiver)
                       .sync();

    this->_pool += this->_is_playing->chain()
                       .perform([weak_player](bool const &is_playing) {
                           if (auto player = weak_player.lock()) {
                               player->_update_playing(is_playing);
                           }
                       })
                       .sync();
}

void audio_player::_setup_rendering_handler(audio_player_ptr const &player) {
    auto weak_player = to_weak(player);

    this->_renderable->set_rendering_handler([weak_player](audio::pcm_buffer *const out_buffer) {
        if (out_buffer->format().is_interleaved()) {
            throw std::invalid_argument("out_buffer is not non-interleaved.");
        }

        auto player = weak_player.lock();
        if (!player) {
            return;
        }

        auto rendering = player->_rendering;
        if (!rendering) {
            return;
        }

        if (!rendering->is_playing) {
            return;
        }

        frame_index_t const begin_frame = rendering->play_frame();
        frame_index_t current_frame = begin_frame;
        uint32_t const out_length = out_buffer->frame_length();
        frame_index_t const next_frame = current_frame + out_length;
        uint32_t const frag_length = rendering->fragment_length();
        audio::format const &out_format = out_buffer->format();
        audio::format const read_format = audio::format{{.sample_rate = out_format.sample_rate(),
                                                         .pcm_format = out_format.pcm_format(),
                                                         .interleaved = false,
                                                         .channel_count = 1}};
        auto const &read_buffer = rendering->get_or_create_read_buffer(read_format, out_length);

        while (current_frame < next_frame) {
            auto const info = audio_utils::processing_info{current_frame, next_frame, frag_length};
            uint32_t const to_frame = uint32_t(current_frame - begin_frame);

            auto each = make_fast_each(out_format.channel_count());
            while (yas_each_next(each)) {
                auto const &idx = yas_each_index(each);

                if (rendering->circular_buffers.size() <= idx) {
                    break;
                }

                read_buffer->clear();
                read_buffer->set_frame_length(info.length);

                auto &circular_buffer = rendering->circular_buffers.at(idx);
                if (auto const result = circular_buffer->read_into_buffer(*read_buffer, current_frame); !result) {
                    out_buffer->clear();
                    return;
                }

                out_buffer->copy_channel_from(*read_buffer,
                                              {.to_channel = idx, .to_begin_frame = to_frame, .length = info.length});

                if (info.next_frag_idx.has_value()) {
                    circular_buffer->rotate_buffers(*info.next_frag_idx);
                }
            }

            current_frame += info.length;
        }

        rendering->update_seeking_after_rendering(next_frame, begin_frame);
    });
}

void audio_player::_update_playing(bool const is_playing) {
    if (auto rendering = this->_rendering) {
        rendering->is_playing.store(is_playing);
    }
    this->_renderable->set_is_rendering(is_playing);
}

void audio_player::_update_rendering() {
    frame_index_t const play_frame = this->play_frame();
    this->_last_play_frame = play_frame;

    this->_rendering = nullptr;

    auto const &format_opt = this->_format->raw();
    if (!format_opt) {
        return;
    }
    auto const &format = *format_opt;

    this->_state_pool.invalidate();
    this->_state_holder->clear();

    auto circular_buffers = this->_make_circular_buffers(format, play_frame);

    for (auto const &buffer : circular_buffers) {
        state_map_holder_ptr_t map_holder = state_map_holder_t::make_shared();
        this->_state_pool += buffer->states_chain().send_to(map_holder).sync();
        this->_state_holder->push_back(std::move(map_holder));
    }

    this->_rendering = std::make_shared<audio_player_rendering>(this->_is_playing->raw(), play_frame, format,
                                                                std::move(circular_buffers));
}

std::vector<audio_circular_buffer_ptr> audio_player::_make_circular_buffers(audio::format const &format,
                                                                            frame_index_t const play_frame) {
    std::size_t const ch_count = this->_ch_count->raw();
    std::vector<channel_index_t> const ch_mapping = this->_actually_ch_mapping();

    sample_rate_t const sample_rate = std::round(format.sample_rate());
    path::timeline const tl_path{this->_root_path, "0", sample_rate};

    std::vector<audio_circular_buffer_ptr> circular_buffers;
    circular_buffers.reserve(ch_count);

    if (auto top_file_idx = audio_player_utils::top_fragment_idx(sample_rate, play_frame);
        top_file_idx && ch_count > 0) {
        auto each = make_fast_each(channel_index_t(ch_count));
        while (yas_each_next(each)) {
            auto const ch_idx = ch_mapping.at(yas_each_index(each));
            auto buffer = audio_circular_buffer::make_shared(
                format, 3, this->_queue, this->_priority,
                [ch_path = path::channel{tl_path, ch_idx}](audio::pcm_buffer &buffer, fragment_index_t const frag_idx) {
                    buffer.clear();

                    auto const frag_path = path::fragment{ch_path, frag_idx};
                    auto const paths_result = file_manager::content_paths_in_directory(frag_path.string());
                    if (!paths_result) {
                        if (paths_result.error() == file_manager::content_paths_error::directory_not_found) {
                            return true;
                        } else {
                            return false;
                        }
                    }

                    auto const &paths = paths_result.value();

                    if (paths.size() == 0) {
                        return true;
                    }

                    auto const &format = buffer.format();
                    std::type_info const &sample_type = yas::to_sample_type(format.pcm_format());
                    if (sample_type == typeid(std::nullptr_t)) {
                        return false;
                    }

                    std::vector<signal_file_info> infos;
                    for (std::string const &path : paths) {
                        if (auto info = to_signal_file_info(path); info->sample_type == sample_type) {
                            infos.emplace_back(std::move(*info));
                        }
                    }

                    if (infos.size() == 0) {
                        return true;
                    }

                    sample_rate_t const sample_rate = std::round(format.sample_rate());
                    frame_index_t const buf_top_frame = frag_idx * sample_rate;

                    for (signal_file_info const &info : infos) {
                        if (auto const result = signal_file::read(info, buffer, buf_top_frame); !result) {
                            return false;
                        }
                    }

                    return true;
                });
            buffer->reload_all_buffers(*top_file_idx);
            circular_buffers.emplace_back(std::move(buffer));
        }
    }

    return circular_buffers;
}

std::vector<channel_index_t> audio_player::_actually_ch_mapping() {
    std::vector<channel_index_t> mapped;

    auto each = make_fast_each(this->_ch_count->raw());
    while (yas_each_next(each)) {
        mapped.push_back(this->_map_ch_idx(yas_each_index(each)));
    }

    return mapped;
}

channel_index_t audio_player::_map_ch_idx(channel_index_t const ch_idx) {
    auto const &mapping = this->_ch_mapping->raw();

    if (ch_idx < mapping.size()) {
        return mapping.at(ch_idx);
    } else {
        return ch_idx;
    }
}

audio_player_ptr audio_player::make_shared(audio_renderable_ptr const &renderable, std::string const &root_path,
                                           std::shared_ptr<task_queue> const &queue, task_priority_t const priority) {
    auto shared = audio_player_ptr(new audio_player{renderable, root_path, queue, priority});
    shared->_prepare(shared);
    return shared;
}
