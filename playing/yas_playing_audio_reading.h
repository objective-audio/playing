//
//  yas_playing_audio_reading.h
//

#pragma once

#include <audio/yas_audio_format.h>
#include <audio/yas_audio_pcm_buffer.h>
#include <playing/yas_playing_audio_reading_protocol.h>
#include <playing/yas_playing_ptr.h>

namespace yas::playing {
struct audio_reading : audio_reading_protocol {
    [[nodiscard]] state_t state() const override;
    [[nodiscard]] audio::pcm_buffer *buffer_on_render() override;

    [[nodiscard]] bool needs_create_on_render(double const sample_rate, audio::pcm_format const,
                                              uint32_t const length) const override;
    void set_creating_on_render(double const sample_rate, audio::pcm_format const, uint32_t const length) override;
    void create_buffer_on_task() override;

    static audio_reading_ptr make_shared();

   private:
    audio_reading();

    std::atomic<state_t> _current_state = state_t::initial;
    audio::pcm_buffer_ptr _buffer = nullptr;
    double _sample_rate = 0;
    audio::pcm_format _pcm_format = audio::pcm_format::float32;
    uint32_t _length = 0;
};
}  // namespace yas::playing
