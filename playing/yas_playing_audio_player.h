//
//  yas_playing_audio_player.h
//

#pragma once

#include <cpp_utils/yas_worker.h>
#include <playing/yas_playing_audio_player_protocol.h>
#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>

namespace yas::playing {
struct audio_player : audio_playable {
    void set_channel_mapping(std::vector<int64_t>);
    void set_playing(bool const) override;
    void seek(frame_index_t const) override;
    void overwrite(channel_index_t const, fragment_index_t const) override;

    [[nodiscard]] std::vector<channel_index_t> const &ch_mapping() const;
    [[nodiscard]] bool is_playing() const override;
    [[nodiscard]] frame_index_t play_frame() const override;

    [[nodiscard]] chaining::chain_sync_t<bool> is_playing_chain() const override;

    struct task_priority {
        uint32_t setup;
        uint32_t rendering;
    };

    static player_ptr make_shared(audio_renderable_ptr const &, std::string const &root_path, workable_ptr const &,
                                  task_priority const &, audio_player_resource_protocol_ptr const &);

   private:
    audio_renderable_ptr const _renderer;
    workable_ptr const _worker;
    task_priority const _priority;

    chaining::value::holder_ptr<bool> _is_playing = chaining::value::holder<bool>::make_shared(false);
    chaining::value::holder_ptr<std::vector<channel_index_t>> const _ch_mapping =
        chaining::value::holder<std::vector<channel_index_t>>::make_shared(std::vector<channel_index_t>{});
    chaining::observer_pool _pool;

    audio_player_resource_protocol_ptr const _resource;

    audio_player(audio_renderable_ptr const &, std::string const &root_path, workable_ptr const &,
                 task_priority const &, audio_player_resource_protocol_ptr const &);
};
}  // namespace yas::playing
