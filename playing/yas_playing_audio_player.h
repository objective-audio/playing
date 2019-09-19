//
//  yas_playing_audio_player.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <cpp_utils/yas_task.h>
#include "yas_playing_audio_player_protocol.h"
#include "yas_playing_loading_state.h"
#include "yas_playing_ptr.h"
#include "yas_playing_types.h"

namespace yas::playing {
class audio_circular_buffer;

struct audio_player {
    void set_ch_mapping(std::vector<int64_t>);
    void set_playing(bool const);
    void seek(frame_index_t const play_frame);
    void reload(channel_index_t const ch_idx, fragment_index_t const frag_idx);

    [[nodiscard]] std::string const &root_path() const;
    [[nodiscard]] std::vector<channel_index_t> const &ch_mapping() const;
    [[nodiscard]] bool is_playing() const;
    [[nodiscard]] frame_index_t play_frame() const;

    [[nodiscard]] chaining::chain_sync_t<bool> is_playing_chain() const;
    [[nodiscard]] state_map_vector_holder_t::chain_t state_chain() const;

    static audio_player_ptr make_shared(audio_renderable_ptr const &renderable, std::string const &root_path,
                                        std::shared_ptr<task_queue> const &queue, task_priority_t const priority);

   private:
    std::string const _root_path;
    chaining::value::holder_ptr<bool> _is_playing = chaining::value::holder<bool>::make_shared(false);
    chaining::value::holder_ptr<std::vector<channel_index_t>> _ch_mapping =
        chaining::value::holder<std::vector<channel_index_t>>::make_shared(std::vector<channel_index_t>{});
    state_map_vector_holder_ptr_t _state_holder = state_map_vector_holder_t::make_shared();

    chaining::observer_pool _pool;
    chaining::observer_pool _state_pool;

    std::shared_ptr<task_queue> const _queue;
    task_priority_t const _priority;
    audio_renderable_ptr _renderable;
    chaining::value::holder_ptr<std::size_t> _ch_count =
        chaining::value::holder<std::size_t>::make_shared(std::size_t(0));
    chaining::value::holder_ptr<std::optional<audio::format>> _format =
        chaining::value::holder<std::optional<audio::format>>::make_shared(std::nullopt);
    chaining::perform_receiver_ptr<> _update_rendering_receiver = nullptr;
    audio_player_rendering_ptr _rendering = nullptr;
    frame_index_t _last_play_frame = 0;

    audio_player(audio_renderable_ptr const &renderable, std::string const &root_path,
                 std::shared_ptr<task_queue> const &queue, task_priority_t const priority);

    void _prepare(audio_player_ptr const &);
    void _setup_chaining(audio_player_ptr const &);
    void _setup_rendering_handler(audio_player_ptr const &);
    void _update_playing(bool const);
    void _update_rendering();
    std::vector<std::shared_ptr<audio_circular_buffer>> _make_circular_buffers(audio::format const &format,
                                                                               frame_index_t const play_frame);
    std::vector<channel_index_t> _actually_ch_mapping();
    channel_index_t _map_ch_idx(channel_index_t const ch_idx);
};
}  // namespace yas::playing
