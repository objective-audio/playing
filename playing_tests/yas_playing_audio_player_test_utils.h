//
//  yas_playing_audio_player_test_utils.h
//

#pragma once

#include <cpp_utils/yas_fast_each.h>
#include <playing/yas_playing_umbrella.h>

#include "yas_playing_test_utils.h"

namespace yas::playing::test {
struct renderer : audio_renderable {
    std::function<void(rendering_f &&)> set_rendering_handler_handler;
    std::function<void(bool)> set_is_rendering_handler;

    void set_rendering_handler(rendering_f &&handler) override {
        this->set_rendering_handler_handler(std::move(handler));
    }

    void set_is_rendering(bool const is_rendering) override {
        this->set_is_rendering_handler(is_rendering);
    }
};

struct resource : audio_player_resource_protocol {
    std::function<void(bool)> set_playing_handler;
    std::function<bool(void)> is_playing_handler;
    std::function<void(frame_index_t)> seek_handler;
    std::function<std::optional<frame_index_t>(void)> pull_seek_frame_handler;
    std::function<void(std::vector<channel_index_t> const &)> set_ch_mapping_handler;
    std::function<std::optional<std::vector<channel_index_t>>(void)> pull_ch_mapping_handler;
    std::function<void(frame_index_t)> set_current_frame_handler;
    std::function<frame_index_t(void)> current_frame_handler;
    std::function<void(element_address &&)> add_overwrite_request_handler;
    std::function<void(overwrite_requests_f const &)> perform_overwrite_requests_handler;
    std::function<void(void)> reset_overwrite_requests_handler;

    audio_reading_protocol_ptr const _reading;
    audio_buffering_protocol_ptr const _buffering;

    resource(audio_reading_protocol_ptr const &reading, audio_buffering_protocol_ptr const &buffering)
        : _reading(reading), _buffering(buffering) {
    }

    audio_reading_protocol_ptr const &reading() const override {
        return this->_reading;
    }

    audio_buffering_protocol_ptr const &buffering() const override {
        return this->_buffering;
    }

    void set_playing_on_main(bool const is_playing) override {
        this->set_playing_handler(is_playing);
    }

    bool is_playing_on_render() const override {
        return this->is_playing_handler();
    }

    void seek_on_main(frame_index_t const frame) override {
        this->seek_handler(frame);
    }

    std::optional<frame_index_t> pull_seek_frame_on_render() override {
        return this->pull_seek_frame_handler();
    }

    void set_channel_mapping_on_main(std::vector<channel_index_t> const &ch_mapping) override {
        this->set_ch_mapping_handler(ch_mapping);
    }

    std::optional<std::vector<channel_index_t>> pull_channel_mapping_on_render() override {
        return this->pull_ch_mapping_handler();
    }

    void set_current_frame_on_render(frame_index_t const frame) override {
        this->set_current_frame_handler(frame);
    }

    frame_index_t current_frame() const override {
        return this->current_frame_handler();
    }

    void add_overwrite_request_on_main(element_address &&address) override {
        this->add_overwrite_request_handler(std::move(address));
    }

    void perform_overwrite_requests_on_render(overwrite_requests_f const &handler) override {
        this->perform_overwrite_requests_handler(handler);
    }

    void reset_overwrite_requests_on_render() override {
        this->reset_overwrite_requests_handler();
    }
};

struct reading : audio_reading_protocol {
    std::function<state_t(void)> state_handler;
    std::function<audio::pcm_buffer *(void)> buffer_handler;
    std::function<bool(double, audio::pcm_format, uint32_t)> needs_create_handler;
    std::function<void(double, audio::pcm_format, uint32_t)> set_creating_handler;
    std::function<void(void)> create_buffer_handler;

    state_t state() const override {
        return this->state_handler();
    }

    audio::pcm_buffer *buffer_on_render() override {
        return this->buffer_handler();
    }

    bool needs_create_on_render(double const sample_rate, audio::pcm_format const pcm_format,
                                uint32_t const length) const override {
        return this->needs_create_handler(sample_rate, pcm_format, length);
    }

    void set_creating_on_render(double const sample_rate, audio::pcm_format const pcm_format,
                                uint32_t const length) override {
        this->set_creating_handler(sample_rate, pcm_format, length);
    }

    void create_buffer_on_task() override {
        this->create_buffer_handler();
    }
};

struct buffering : audio_buffering_protocol {
    std::function<setup_state_t(void)> setup_state_handler;
    std::function<rendering_state_t(void)> rendering_state_handler;
    std::function<std::size_t(void)> element_count_handler;
    std::function<std::size_t(void)> channel_count_handler;
    std::function<sample_rate_t(void)> fragment_length_handler;
    std::function<void(double, audio::pcm_format, uint32_t)> set_creating_handler;
    std::function<bool(double, audio::pcm_format, uint32_t)> needs_create_handler;
    std::function<void(void)> create_buffer_handler;
    std::function<void(frame_index_t, std::optional<std::vector<channel_index_t>> &&)> set_all_writing_handler;
    std::function<void(void)> write_all_elements_handler;
    std::function<void(fragment_index_t)> advance_handler;
    std::function<bool(void)> write_elements_if_needed_handler;
    std::function<void(element_address const &)> overwrite_element_handler;
    std::function<bool(audio::pcm_buffer *, channel_index_t, frame_index_t)> read_into_buffer_handler;

    setup_state_t setup_state() const override {
        return this->setup_state_handler();
    }

    rendering_state_t rendering_state() const override {
        return this->rendering_state_handler();
    }

    std::size_t element_count() const override {
        return this->element_count_handler();
    }

    std::size_t channel_count_on_render() const override {
        return this->channel_count_handler();
    }

    sample_rate_t fragment_length_on_render() const override {
        return this->fragment_length_handler();
    }

    void set_creating_on_render(double const sample_rate, audio::pcm_format const &pcm_format,
                                uint32_t const ch_count) override {
        this->set_creating_handler(sample_rate, pcm_format, ch_count);
    }

    bool needs_create_on_render(double const sample_rate, audio::pcm_format const &pcm_format,
                                uint32_t const ch_count) override {
        return this->needs_create_handler(sample_rate, pcm_format, ch_count);
    }

    void create_buffer_on_task() override {
        this->create_buffer_handler();
    }

    void set_all_writing_on_render(frame_index_t const frame,
                                   std::optional<std::vector<channel_index_t>> &&ch_mapping) override {
        this->set_all_writing_handler(frame, std::move(ch_mapping));
    }

    void write_all_elements_on_task() override {
        this->write_all_elements_handler();
    }

    void advance_on_render(fragment_index_t const frag_idx) override {
        this->advance_handler(frag_idx);
    }

    bool write_elements_if_needed_on_task() override {
        return this->write_elements_if_needed_handler();
    }

    void overwrite_element_on_render(element_address const &address) override {
        this->overwrite_element_handler(address);
    }

    bool read_into_buffer_on_render(audio::pcm_buffer *buffer, channel_index_t const ch_idx,
                                    frame_index_t const frame_idx) override {
        return this->read_into_buffer_handler(buffer, ch_idx, frame_idx);
    }
};

struct audio_player_cpp {
    static double constexpr sample_rate = 4;
    static audio::pcm_format constexpr pcm_format = audio::pcm_format::int16;
    static uint32_t constexpr ch_count = 3;
    static std::size_t constexpr length = 2;

    worker_stub_ptr const worker = worker_stub::make_shared();
    std::shared_ptr<test::renderer> const renderer = std::make_shared<test::renderer>();
    std::shared_ptr<test::reading> const reading = std::make_shared<test::reading>();
    std::shared_ptr<test::buffering> const buffering = std::make_shared<test::buffering>();
    std::shared_ptr<test::resource> const resource = std::make_shared<test::resource>(reading, buffering);

    audio_player_ptr player = nullptr;
    audio_renderable::rendering_f rendering_handler = nullptr;
    audio::pcm_buffer_ptr reading_buffer = nullptr;

    static audio::format make_format() {
        return audio::format{{.sample_rate = sample_rate, .pcm_format = pcm_format, .channel_count = ch_count}};
    }

    static audio::pcm_buffer make_out_buffer() {
        return audio::pcm_buffer{make_format(), length};
    }

    static void fill_buffer(audio::pcm_buffer *buffer, channel_index_t const ch_idx, frame_index_t const begin_frame) {
        auto *data = buffer->data_ptr_at_index<int16_t>(0);

        auto each = make_fast_each(buffer->frame_length());
        while (yas_each_next(each)) {
            auto const &idx = yas_each_index(each);
            data[idx] = ch_idx * 1000 + begin_frame + idx;
        }
    }

    void setup_initial() {
        this->resource->set_ch_mapping_handler = [](std::vector<channel_index_t> const &ch_mapping) {};
        this->resource->set_playing_handler = [](bool is_playing) {};
        this->renderer->set_rendering_handler_handler = [this](audio_renderable::rendering_f &&handler) {
            this->rendering_handler = std::move(handler);
        };
        this->renderer->set_is_rendering_handler = [](bool is_rendering) {};

        audio_player_task_priority const priority{.setup = 100, .rendering = 101};

        this->player =
            audio_player::make_shared(this->renderer, test_utils::root_path(), this->worker, priority, this->resource);
    }

    void skip_reading() {
        this->setup_initial();

        auto const &reading = this->reading;

        reading->state_handler = [] { return playing::audio_reading_state::rendering; };
        reading->set_creating_handler = [](double, audio::pcm_format, uint32_t) {};
        reading->needs_create_handler = [](double, audio::pcm_format, uint32_t) { return false; };
    }

    void skip_buffering_setup() {
        this->skip_reading();

        auto const &buffering = this->buffering;

        buffering->setup_state_handler = [] { return audio_buffering_setup_state::rendering; };
        buffering->set_creating_handler = [](double, audio::pcm_format, uint32_t) {};
        buffering->needs_create_handler = [](double, audio::pcm_format, uint32_t) { return false; };
    }

    void skip_buffering_rendering() {
        this->skip_buffering_setup();

        this->buffering->rendering_state_handler = [] { return audio_buffering_rendering_state::advancing; };
    }

    void skip_ch_mapping() {
        this->skip_buffering_rendering();

        this->resource->pull_seek_frame_handler = [] { return std::nullopt; };
        this->resource->pull_ch_mapping_handler = [] { return std::nullopt; };
    }

    void skip_playing() {
        this->skip_ch_mapping();

        this->reading_buffer = std::make_shared<audio::pcm_buffer>(this->make_format(), audio_player_cpp::length);

        this->resource->perform_overwrite_requests_handler = [](test::resource::overwrite_requests_f const &) {};
        this->resource->is_playing_handler = [] { return true; };
        this->reading->buffer_handler = [this] { return this->reading_buffer.get(); };
    }

    void reset() {
        this->player = nullptr;
        this->rendering_handler = nullptr;
        this->reading_buffer = nullptr;
    }
};
}  // namespace yas::playing::test
