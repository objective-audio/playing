//
//  yas_playing_test_audio_renderer.h
//

#pragma once

#include <chaining/yas_chaining_umbrella.h>
#include <playing/yas_playing_audio_player_protocol.h>

namespace yas::playing::test_utils {
class test_audio_renderer;
using test_audio_renderer_ptr = std::shared_ptr<test_audio_renderer>;

struct test_audio_renderer : audio_coordinator_renderable {
    void set_pcm_format(audio::pcm_format const);
    void set_channel_count(uint32_t const);
    void set_sample_rate(double const);

    void set_rendering_handler(rendering_f &&) override;
    void set_is_rendering(bool const) override;

    [[nodiscard]] proc::sample_rate_t sample_rate() const override;
    [[nodiscard]] audio::pcm_format pcm_format() const override;
    [[nodiscard]] std::size_t channel_count() const override;

    [[nodiscard]] chaining::chain_sync_t<audio_configuration> configuration_chain() const override;

    [[nodiscard]] bool render_on_bg(audio::pcm_buffer *const);

    [[nodiscard]] static test_audio_renderer_ptr make_shared();

   private:
    chaining::value::holder_ptr<proc::sample_rate_t> _sample_rate =
        chaining::value::holder<proc::sample_rate_t>::make_shared(proc::sample_rate_t{0});
    chaining::value::holder_ptr<audio::pcm_format> _pcm_format =
        chaining::value::holder<audio::pcm_format>::make_shared(audio::pcm_format::float32);
    chaining::value::holder_ptr<std::size_t> _channel_count =
        chaining::value::holder<std::size_t>::make_shared(std::size_t(0));
    chaining::value::holder_ptr<audio_configuration> const _configuration =
        chaining::value::holder<audio_configuration>::make_shared(
            {.sample_rate = 0, .pcm_format = audio::pcm_format::float32, .channel_count = 0});
    std::atomic<bool> _is_rendering = false;
    audio_renderable::rendering_f _rendering_handler;

    chaining::observer_pool _pool;

    test_audio_renderer();
};
}  // namespace yas::playing::test_utils
