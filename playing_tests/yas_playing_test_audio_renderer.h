//
//  yas_playing_test_audio_renderer.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <playing/yas_playing_audio_player_protocol.h>

namespace yas::playing::test_utils {
class test_audio_renderer;
using test_audio_renderer_ptr = std::shared_ptr<test_audio_renderer>;

struct test_audio_renderer : audio_renderable {
    void set_pcm_format(audio::pcm_format const);
    void set_channel_count(uint32_t const);
    void set_sample_rate(double const);

    void render(audio::pcm_buffer &buffer);

    static test_audio_renderer_ptr make_shared();

   private:
    class impl;

    std::unique_ptr<impl> _impl;

    test_audio_renderer();

    void set_rendering_handler(rendering_f) override;
    chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() override;
    chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() override;
    chaining::chain_sync_t<std::size_t> chain_channel_count() override;
    void set_is_rendering(bool const) override;
};
}  // namespace yas::playing::test_utils
