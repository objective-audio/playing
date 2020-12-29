//
//  yas_playing_player.h
//

#pragma once

#include <cpp_utils/yas_worker.h>
#include <playing/yas_playing_player_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>

namespace yas::playing {
struct player : playable {
    void set_channel_mapping(channel_mapping_ptr const &) override;
    void set_playing(bool const) override;
    void seek(frame_index_t const) override;
    void overwrite(channel_index_t const, fragment_index_t const) override;

    [[nodiscard]] channel_mapping_ptr const &channel_mapping() const override;
    [[nodiscard]] bool is_playing() const override;
    [[nodiscard]] frame_index_t current_frame() const override;

    [[nodiscard]] chaining::chain_sync_t<bool> is_playing_chain() const override;

    static player_ptr make_shared(renderable_ptr const &, std::string const &root_path, workable_ptr const &,
                                  task_priority_t const &, player_resource_protocol_ptr const &);

   private:
    renderable_ptr const _renderer;
    workable_ptr const _worker;
    task_priority_t const _priority;
    player_resource_protocol_ptr const _resource;

    chaining::value::holder_ptr<bool> _is_playing = chaining::value::holder<bool>::make_shared(false);
    chaining::value::holder_ptr<channel_mapping_ptr> const _ch_mapping;
    chaining::observer_pool _pool;

    player(renderable_ptr const &, std::string const &root_path, workable_ptr const &, task_priority_t const &,
           player_resource_protocol_ptr const &);
};
}  // namespace yas::playing
