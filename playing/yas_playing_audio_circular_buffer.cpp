//
//  yas_playing_audio_channel_buffer.mm
//

#include "yas_playing_audio_circular_buffer.h"
#include <cpp_utils/yas_fast_each.h>
#include <cpp_utils/yas_thread.h>
#include <mutex>
#include "yas_playing_math.h"
#include "yas_playing_path.h"
#include "yas_playing_types.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing {
std::vector<audio_buffer::ptr> make_audio_buffers(audio::format const &format, std::size_t const count,
                                                  audio_buffer::state_changed_f handler) {
    auto const length = static_cast<uint32_t>(format.sample_rate());

    std::vector<audio_buffer::ptr> result;
    result.reserve(count);

    auto each = make_fast_each(count);
    while (yas_each_next(each)) {
        result.emplace_back(make_audio_buffer(audio::pcm_buffer{format, length}, handler));
    }

    return result;
}
}  // namespace yas::playing

audio_circular_buffer::audio_circular_buffer(audio::format const &format, std::size_t const count, task_queue &&queue,
                                             task_priority_t const priority, audio_buffer::load_f &&load_handler)
    : _frag_length(static_cast<length_t>(format.sample_rate())),
      _queue(std::move(queue)),
      _priority(priority),
      _buffer_count(count),
      _format(format),
      _load_handler_ptr(std::make_shared<audio_buffer::load_f>(std::move(load_handler))) {
}

audio_circular_buffer::read_result_t audio_circular_buffer::read_into_buffer(audio::pcm_buffer &out_buffer,
                                                                             frame_index_t const play_frame) {
    if (auto const result = this->_current_buffer()->read_into_buffer(out_buffer, play_frame)) {
        return read_result_t{nullptr};
    } else {
        return read_result_t{read_error::read_from_container_failed};
    }
}

void audio_circular_buffer::rotate_buffers(fragment_index_t const top_frag_idx) {
    std::lock_guard<std::recursive_mutex> lock(this->_loading_mutex);

    if (auto &buffer = this->_current_buffer(); buffer->fragment_idx() == top_frag_idx - 1) {
        this->_rotate_buffers();
        fragment_index_t const loading_frag_idx = top_frag_idx + this->_buffer_count - 1;
        this->_load_container(buffer, loading_frag_idx);
    } else {
        this->reload_all_buffers(top_frag_idx);
    }
}

void audio_circular_buffer::reload_all_buffers(fragment_index_t const top_frag_idx) {
    fragment_index_t load_frag_idx = top_frag_idx;

    std::lock_guard<std::recursive_mutex> lock(this->_loading_mutex);

    auto const current_idx = this->_current_idx.load();
    auto each = make_fast_each(this->_buffer_count);
    while (yas_each_next(each)) {
        auto const idx = (current_idx + yas_each_index(each)) % this->_buffer_count;
        auto &buffer = this->_buffers.at(idx);
        this->_load_container(buffer, load_frag_idx);
        ++load_frag_idx;
    }
}

void audio_circular_buffer::reload_if_needed(fragment_index_t const frag_idx) {
    std::lock_guard<std::recursive_mutex> lock(this->_loading_mutex);

    for (auto &container_ptr : this->_buffers) {
        if (auto const frag_idx_opt = container_ptr->fragment_idx(); *frag_idx_opt == frag_idx) {
            this->_load_container(container_ptr, frag_idx);
        }
    }
}

void audio_circular_buffer::_load_container(audio_buffer::ptr buffer_ptr, fragment_index_t const frag_idx) {
    std::lock_guard<std::recursive_mutex> lock(this->_loading_mutex);

    buffer_ptr->prepare_loading(frag_idx);

    audio_circular_buffer::wptr weak = this->shared_from_this();

    task task{[weak, buffer_ptr, frag_idx, load_handler_ptr = this->_load_handler_ptr](yas::task const &) {
                  buffer_ptr->load(frag_idx, *load_handler_ptr);
              },
              task_option_t{.push_cancel_id = buffer_ptr->identifier, .priority = this->_priority}};

    this->_queue.push_back(std::move(task));
}

audio_buffer::ptr const &audio_circular_buffer::_current_buffer() {
    return this->_buffers.at(this->_current_idx.load());
}

void audio_circular_buffer::_rotate_buffers() {
    this->_current_idx.store((this->_current_idx.load() + 1) % this->_buffer_count);
}

audio_circular_buffer::state_map_t const &audio_circular_buffer::states() const {
    assert(thread::is_main());
    return this->_states_holder.raw();
}

audio_circular_buffer::state_map_holder_t::chain_t audio_circular_buffer::states_chain() const {
    assert(thread::is_main());
    return this->_states_holder.chain();
}

std::optional<fragment_index_t> audio_circular_buffer::_index_of(uintptr_t const identifier) {
    auto &buffers = this->_buffers;
    auto each = make_fast_each(buffers.size());
    while (yas_each_next(each)) {
        auto const &idx = yas_each_index(each);
        if (buffers.at(idx)->identifier.identifier() == identifier) {
            return idx;
        }
    }
    return std::nullopt;
}

#pragma mark -

namespace yas::playing {
struct audio_circular_buffer_factory : audio_circular_buffer {
    audio_circular_buffer_factory(audio::format const &format, std::size_t const container_count, task_queue &&queue,
                                  task_priority_t const priority, audio_buffer::load_f &&handler)
        : audio_circular_buffer(format, container_count, std::move(queue), priority, std::move(handler)) {
    }

    void prepare(std::shared_ptr<audio_circular_buffer_factory> &factory) {
        std::weak_ptr<audio_circular_buffer_factory> weak_factory = factory;
        this->_buffers =
            make_audio_buffers(this->_format, this->_buffer_count,
                               [weak_factory](uintptr_t const identifier, audio_buffer::state::ptr const state) {
                                   dispatch_async(dispatch_get_main_queue(), ^{
                                       if (auto factory = weak_factory.lock()) {
                                           if (auto const idx = factory->_index_of(identifier)) {
                                               factory->_states_holder.insert_or_replace(*idx, std::move(*state));
                                           }
                                       }
                                   });
                               });
    }
};
}  // namespace yas::playing

audio_circular_buffer::ptr playing::make_audio_circular_buffer(audio::format const &format,
                                                               std::size_t const container_count, task_queue queue,
                                                               task_priority_t const priority,
                                                               audio_buffer::load_f handler) {
    auto ptr = std::make_shared<audio_circular_buffer_factory>(format, container_count, std::move(queue), priority,
                                                               std::move(handler));
    ptr->prepare(ptr);
    return ptr;
}
