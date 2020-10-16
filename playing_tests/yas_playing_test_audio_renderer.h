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

    void render(audio::pcm_buffer *const);

    static test_audio_renderer_ptr make_shared();

   private:
    chaining::value::holder_ptr<proc::sample_rate_t> _sample_rate =
        chaining::value::holder<proc::sample_rate_t>::make_shared(proc::sample_rate_t{0});
    chaining::value::holder_ptr<audio::pcm_format> _pcm_format =
        chaining::value::holder<audio::pcm_format>::make_shared(audio::pcm_format::float32);
    chaining::value::holder_ptr<std::size_t> _channel_count =
        chaining::value::holder<std::size_t>::make_shared(std::size_t(0));
    std::atomic<bool> _is_rendering = false;
    audio_renderable::rendering_f _rendering_handler;
    std::recursive_mutex _rendering_mutex;

    test_audio_renderer();

    void set_rendering_handler(rendering_f &&) override;
    chaining::chain_sync_t<proc::sample_rate_t> chain_sample_rate() override;
    chaining::chain_sync_t<audio::pcm_format> chain_pcm_format() override;
    chaining::chain_sync_t<std::size_t> chain_channel_count() override;
    void set_is_rendering(bool const) override;
};
}  // namespace yas::playing::test_utils
