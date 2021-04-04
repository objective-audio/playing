//
//  yas_playing_player.h
//

#pragma once

#include <cpp_utils/yas_worker.h>
#include <playing/yas_playing_channel_mapping.h>
#include <playing/yas_playing_player_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>

namespace yas::playing {
struct player final : playable {
    void set_identifier(std::string const &) override;
    void set_channel_mapping(playing::channel_mapping const &) override;
    void set_playing(bool const) override;
    void seek(frame_index_t const) override;
    void overwrite(std::optional<channel_index_t> const file_ch_idx, fragment_range const) override;

    [[nodiscard]] std::string const &identifier() const override;
    [[nodiscard]] playing::channel_mapping channel_mapping() const override;
    [[nodiscard]] bool is_playing() const override;
    [[nodiscard]] frame_index_t current_frame() const override;

    [[nodiscard]] observing::syncable observe_is_playing(std::function<void(bool const &)> &&) override;

    static player_ptr make_shared(std::string const &root_path, renderable_ptr const &, workable_ptr const &,
                                  task_priority_t const &, player_resource_protocol_ptr const &);

   private:
    renderable_ptr const _renderer;
    workable_ptr const _worker;
    task_priority_t const _priority;
    player_resource_protocol_ptr const _resource;

    observing::value::holder_ptr<bool> _is_playing = observing::value::holder<bool>::make_shared(false);
    playing::channel_mapping _ch_mapping;
    std::string _identifier;
    observing::canceller_pool _pool;

    player(std::string const &root_path, renderable_ptr const &, workable_ptr const &, task_priority_t const &,
           player_resource_protocol_ptr const &);
};
}  // namespace yas::playing
