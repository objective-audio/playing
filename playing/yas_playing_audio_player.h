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
    class impl;

    std::unique_ptr<impl> _impl;

    audio_player(audio_renderable_ptr const &renderable, std::string const &root_path,
                 std::shared_ptr<task_queue> const &queue, task_priority_t const priority);

    void _prepare(audio_player_ptr const &);
};
}  // namespace yas::playing
