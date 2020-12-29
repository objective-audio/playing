//
//  yas_playing_circular_buffer_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>
#import <thread>
#import "yas_playing_test_utils.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::test {
static uint32_t const ch_count = 2;
static uint32_t const element_count = 3;
static sample_rate_t const sample_rate = 4;
static audio::pcm_format const pcm_format = audio::pcm_format::int16;
static audio::format const format{
    {.sample_rate = sample_rate, .pcm_format = pcm_format, .channel_count = 1, .interleaved = false}};
static path::timeline tl_path{
    .root_path = test_utils::root_path(), .identifier = "0", .sample_rate = static_cast<sample_rate_t>(sample_rate)};

static path::channel channel_path(channel_index_t const ch_idx) {
    return path::channel{test::tl_path, ch_idx};
}

struct channel : buffering_channel_protocol {
    std::size_t element_count;
    audio::format format;
    sample_rate_t frag_length;

    channel(std::size_t const element_count, audio::format const &format, sample_rate_t const frag_length)
        : element_count(element_count), format(format), frag_length(frag_length) {
    }

    std::function<bool()> write_elements_handler;
    std::function<void(path::channel const &, fragment_index_t const)> write_all_elements_handler;
    std::function<void(fragment_index_t const, fragment_index_t const)> advance_handler;
    std::function<void(fragment_index_t const)> overwrite_element_handler;
    std::function<bool(audio::pcm_buffer *, frame_index_t const)> read_into_buffer_handler;

    bool write_elements_if_needed_on_task() {
        return this->write_elements_handler();
    }

    void write_all_elements_on_task(path::channel const &ch_path, fragment_index_t const top_frag_idx) {
        this->write_all_elements_handler(ch_path, top_frag_idx);
    }

    void advance_on_render(fragment_index_t const prev_frag_idx, fragment_index_t const new_frag_idx) {
        this->advance_handler(prev_frag_idx, new_frag_idx);
    }

    void overwrite_element_on_render(fragment_index_t const frag_idx) {
        this->overwrite_element_handler(frag_idx);
    }

    bool read_into_buffer_on_render(audio::pcm_buffer *out_buffer, frame_index_t const frame) {
        return this->read_into_buffer_handler(out_buffer, frame);
    }
};

struct audio_buffering_cpp {
    std::shared_ptr<buffering_resource> buffering = nullptr;
    std::vector<std::shared_ptr<test::channel>> channels;

    void setup_rendering() {
        std::vector<std::shared_ptr<test::channel>> channels;

        auto const buffering = buffering_resource::make_shared(
            test::element_count, test_utils::root_path(),
            [&channels](std::size_t const element_count, audio::format const &format, sample_rate_t const frag_length) {
                auto channel = std::make_shared<test::channel>(element_count, format, frag_length);
                channels.emplace_back(channel);
                return channel;
            });

        std::thread{[&buffering] {
            buffering->set_creating_on_render(test::sample_rate, test::pcm_format, test::ch_count);
        }}.join();

        std::thread{[&buffering] { buffering->create_buffer_on_task(); }}.join();

        XCTAssertEqual(buffering->setup_state(), audio_buffering_setup_state::rendering);

        this->buffering = buffering;
        this->channels = std::move(channels);
    }

    void setup_advancing() {
        std::vector<std::shared_ptr<test::channel>> channels;

        auto const buffering = buffering_resource::make_shared(
            test::element_count, test_utils::root_path(),
            [&channels](std::size_t const element_count, audio::format const &format, sample_rate_t const frag_length) {
                auto channel = std::make_shared<test::channel>(element_count, format, frag_length);
                channels.emplace_back(channel);
                return channel;
            });

        std::thread{[&buffering] {
            buffering->set_creating_on_render(test::sample_rate, test::pcm_format, test::ch_count);
        }}.join();

        std::thread{[&buffering] { buffering->create_buffer_on_task(); }}.join();

        auto each = make_fast_each(test::ch_count);
        while (yas_each_next(each)) {
            channels.at(yas_each_index(each))->write_all_elements_handler = [](path::channel const &,
                                                                               fragment_index_t const) {};
        }

        std::thread{[&buffering] { buffering->set_all_writing_on_render(0, std::nullopt); }}.join();

        std::thread{[&buffering] { buffering->write_all_elements_on_task(); }}.join();

        XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::advancing);

        this->buffering = buffering;
        this->channels = std::move(channels);
    }
};
}

@interface yas_playing_buffering_tests : XCTestCase

@end

@implementation yas_playing_buffering_tests {
    test::audio_buffering_cpp _cpp;
}

- (void)tearDown {
    self->_cpp = test::audio_buffering_cpp{};
    [super tearDown];
}

- (void)test_setup_state {
    std::vector<std::shared_ptr<test::channel>> channels;

    auto const buffering = buffering_resource::make_shared(
        test::element_count, test_utils::root_path(),
        [&channels](std::size_t const element_count, audio::format const &format, sample_rate_t const frag_length) {
            auto channel = std::make_shared<test::channel>(element_count, format, frag_length);
            channels.emplace_back(channel);
            return channel;
        });

    XCTAssertEqual(buffering->setup_state(), audio_buffering_setup_state::initial);

    buffering->set_creating_on_render(test::sample_rate, test::pcm_format, test::ch_count);

    XCTAssertEqual(buffering->setup_state(), audio_buffering_setup_state::creating);
    XCTAssertEqual(channels.size(), 0);

    buffering->create_buffer_on_task();

    XCTAssertEqual(buffering->setup_state(), audio_buffering_setup_state::rendering);
    XCTAssertEqual(channels.size(), 2);

    XCTAssertEqual(channels.at(0)->element_count, 3);
    XCTAssertEqual(channels.at(0)->format, test::format);
    XCTAssertEqual(channels.at(0)->frag_length, test::sample_rate);
    XCTAssertEqual(channels.at(1)->element_count, 3);
    XCTAssertEqual(channels.at(1)->format, test::format);
    XCTAssertEqual(channels.at(1)->frag_length, test::sample_rate);

    channels.clear();

    buffering->set_creating_on_render(test::sample_rate, test::pcm_format, test::ch_count);

    XCTAssertEqual(buffering->setup_state(), audio_buffering_setup_state::creating);
    XCTAssertEqual(channels.size(), 0);
}

- (void)test_rendering_state {
    std::vector<std::shared_ptr<test::channel>> channels;

    auto const buffering = buffering_resource::make_shared(
        test::element_count, test_utils::root_path(),
        [&channels](std::size_t const element_count, audio::format const &format, sample_rate_t const frag_length) {
            auto channel = std::make_shared<test::channel>(element_count, format, frag_length);
            channels.emplace_back(channel);
            return channel;
        });

    std::thread{[&buffering] {
        buffering->set_creating_on_render(test::sample_rate, test::pcm_format, test::ch_count);
    }}.join();

    std::thread{[&buffering] { buffering->create_buffer_on_task(); }}.join();

    channels.at(0)->write_all_elements_handler = [](path::channel const &ch_path, fragment_index_t const top_frag_idx) {
    };
    channels.at(1)->write_all_elements_handler = [](path::channel const &ch_path, fragment_index_t const top_frag_idx) {
    };

    XCTAssertEqual(buffering->setup_state(), audio_buffering_setup_state::rendering);
    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::waiting);

    buffering->set_all_writing_on_render(0, std::nullopt);

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::all_writing);

    buffering->write_all_elements_on_task();

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::advancing);

    buffering->set_all_writing_on_render(20, std::nullopt);

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::all_writing);
}

- (void)test_channel_count_and_fragment_length {
    self->_cpp.setup_advancing();

    auto const buffering = self->_cpp.buffering;

    XCTAssertEqual(buffering->channel_count_on_render(), test::ch_count);
    XCTAssertEqual(buffering->fragment_length_on_render(), test::sample_rate);
}

- (void)test_needs_create {
    self->_cpp.setup_rendering();

    auto const buffering = self->_cpp.buffering;

    XCTAssertFalse(buffering->needs_create_on_render(test::sample_rate, test::pcm_format, test::ch_count));

    XCTAssertTrue(buffering->needs_create_on_render(5, test::pcm_format, test::ch_count));
    XCTAssertTrue(buffering->needs_create_on_render(test::sample_rate, audio::pcm_format::other, test::ch_count));
    XCTAssertTrue(buffering->needs_create_on_render(test::sample_rate, test::pcm_format, 3));
}

- (void)test_set_all_writing_from_waiting {
    self->_cpp.setup_rendering();

    auto const buffering = self->_cpp.buffering;

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::waiting);

    buffering->set_all_writing_on_render(100, std::nullopt);

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::all_writing);
    XCTAssertEqual(buffering->all_writing_frame_for_test(), 100);
    XCTAssertEqual(buffering->ch_mapping_for_test()->indices.size(), 0);
}

- (void)test_set_all_writing_from_advancing {
    self->_cpp.setup_advancing();

    auto const buffering = self->_cpp.buffering;

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::advancing);

    buffering->set_all_writing_on_render(200, channel_mapping::make_shared({1, 0}));

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::all_writing);
    XCTAssertEqual(buffering->all_writing_frame_for_test(), 200);
    XCTAssertEqual(buffering->ch_mapping_for_test()->indices, (std::vector<channel_index_t>{1, 0}));
}

- (void)test_advance {
    self->_cpp.setup_advancing();

    auto const buffering = self->_cpp.buffering;

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::advancing);

    std::vector<std::pair<fragment_index_t, fragment_index_t>> called_advance_0;
    std::vector<std::pair<fragment_index_t, fragment_index_t>> called_advance_1;

    auto &channels = self->_cpp.channels;
    channels.at(0)->advance_handler = [&called_advance_0](fragment_index_t const prev_idx,
                                                          fragment_index_t const new_idx) {
        called_advance_0.emplace_back(prev_idx, new_idx);
    };
    channels.at(1)->advance_handler = [&called_advance_1](fragment_index_t const prev_idx,
                                                          fragment_index_t const new_idx) {
        called_advance_1.emplace_back(prev_idx, new_idx);
    };

    buffering->advance_on_render(10);

    XCTAssertEqual(called_advance_0.size(), 1);
    XCTAssertEqual(called_advance_0.at(0).first, 7, @"引数に渡したidxからelement_countを引いた値");
    XCTAssertEqual(called_advance_0.at(0).second, 10, @"引数に渡したidxそのまま");
    XCTAssertEqual(called_advance_1.size(), 1);
    XCTAssertEqual(called_advance_1.at(0).first, 7, @"引数に渡したidxからelement_countを引いた値");
    XCTAssertEqual(called_advance_1.at(0).second, 10, @"引数に渡したidxそのまま");
}

- (void)test_write_elements_if_needed {
    self->_cpp.setup_advancing();

    auto const buffering = self->_cpp.buffering;

    bool result0 = false;
    bool result1 = false;

    std::size_t called_count_0;
    std::size_t called_count_1;

    auto &channels = self->_cpp.channels;
    channels.at(0)->write_elements_handler = [&result0, &called_count_0]() {
        ++called_count_0;
        return result0;
    };
    channels.at(1)->write_elements_handler = [&result1, &called_count_1]() {
        ++called_count_1;
        return result1;
    };

    XCTAssertFalse(buffering->write_elements_if_needed_on_task());

    XCTAssertEqual(called_count_0, 1);
    XCTAssertEqual(called_count_1, 1);

    result0 = true;
    result1 = false;

    XCTAssertTrue(buffering->write_elements_if_needed_on_task());

    XCTAssertEqual(called_count_0, 2);
    XCTAssertEqual(called_count_1, 2);

    result0 = false;
    result1 = true;

    XCTAssertTrue(buffering->write_elements_if_needed_on_task());

    XCTAssertEqual(called_count_0, 3);
    XCTAssertEqual(called_count_1, 3);

    result0 = true;
    result1 = true;

    XCTAssertTrue(buffering->write_elements_if_needed_on_task());

    XCTAssertEqual(called_count_0, 4);
    XCTAssertEqual(called_count_1, 4);
}

- (void)test_write_all_elements {
    self->_cpp.setup_rendering();

    auto const &buffering = self->_cpp.buffering;
    auto &channels = self->_cpp.channels;

    std::vector<std::pair<path::channel, fragment_index_t>> called_channel_0;
    std::vector<std::pair<path::channel, fragment_index_t>> called_channel_1;

    channels.at(0)->write_all_elements_handler = [&called_channel_0](path::channel const &ch_path,
                                                                     fragment_index_t const top_frag_idx) {
        called_channel_0.emplace_back(ch_path, top_frag_idx);
    };
    channels.at(1)->write_all_elements_handler = [&called_channel_1](path::channel const &ch_path,
                                                                     fragment_index_t const top_frag_idx) {
        called_channel_1.emplace_back(ch_path, top_frag_idx);
    };

    buffering->set_all_writing_on_render(0, std::nullopt);

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::all_writing);

    XCTAssertEqual(called_channel_0.size(), 0);
    XCTAssertEqual(called_channel_1.size(), 0);

    buffering->write_all_elements_on_task();

    XCTAssertEqual(buffering->rendering_state(), audio_buffering_rendering_state::advancing);

    XCTAssertEqual(called_channel_0.size(), 1);
    XCTAssertEqual(called_channel_0.at(0).first, test::channel_path(0));
    XCTAssertEqual(called_channel_0.at(0).second, 0);
    XCTAssertEqual(called_channel_1.size(), 1);
    XCTAssertEqual(called_channel_1.at(0).first, test::channel_path(1));
    XCTAssertEqual(called_channel_1.at(0).second, 0);

    std::thread{[&buffering] {
        buffering->set_all_writing_on_render(10, channel_mapping::make_shared({1, 0}));
    }}.join();
    std::thread{[&buffering] { buffering->write_all_elements_on_task(); }}.join();

    XCTAssertEqual(called_channel_0.size(), 2);
    XCTAssertEqual(called_channel_0.at(1).first, test::channel_path(1));
    XCTAssertEqual(called_channel_0.at(1).second, 2, @"10(frame) / 4(frag_length) = 2");
    XCTAssertEqual(called_channel_1.size(), 2);
    XCTAssertEqual(called_channel_1.at(1).first, test::channel_path(0));
    XCTAssertEqual(called_channel_1.at(1).second, 2);

    std::thread{[&buffering] { buffering->set_all_writing_on_render(20, std::nullopt); }}.join();
    std::thread{[&buffering] { buffering->write_all_elements_on_task(); }}.join();

    XCTAssertEqual(called_channel_0.size(), 3);
    XCTAssertEqual(called_channel_0.at(2).first, test::channel_path(1), @"nulloptの場合は最後のch_pathが使われる");
    XCTAssertEqual(called_channel_0.at(2).second, 5, @"20(frame) / 4(frag_length) = 5");
    XCTAssertEqual(called_channel_1.size(), 3);
    XCTAssertEqual(called_channel_1.at(2).first, test::channel_path(0));
    XCTAssertEqual(called_channel_1.at(2).second, 5);
}

- (void)test_overwrite_element {
    self->_cpp.setup_advancing();

    auto const &buffering = self->_cpp.buffering;
    auto &channels = self->_cpp.channels;

    std::vector<fragment_index_t> called0;
    std::vector<fragment_index_t> called1;

    channels.at(0)->overwrite_element_handler = [&called0](fragment_index_t const frag_idx) {
        called0.emplace_back(frag_idx);
    };
    channels.at(1)->overwrite_element_handler = [&called1](fragment_index_t const frag_idx) {
        called1.emplace_back(frag_idx);
    };

    buffering->overwrite_element_on_render({.channel_index = 0, .fragment_index = 0});

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called0.at(0), 0);
    XCTAssertEqual(called1.size(), 0);

    buffering->overwrite_element_on_render({.channel_index = 1, .fragment_index = 1});

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called1.size(), 1);
    XCTAssertEqual(called1.at(0), 1);

    buffering->overwrite_element_on_render({.channel_index = 2, .fragment_index = 2});
    buffering->overwrite_element_on_render({.channel_index = -1, .fragment_index = -1});

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called1.size(), 1);

    std::thread{[&buffering] { buffering->set_all_writing_on_render(0, channel_mapping::make_shared({2, 3})); }}.join();
    std::thread{[&buffering] { buffering->write_all_elements_on_task(); }}.join();

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called1.size(), 1);

    buffering->overwrite_element_on_render({.channel_index = 3, .fragment_index = 3});

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called1.size(), 2);
    XCTAssertEqual(called1.at(1), 3);

    buffering->overwrite_element_on_render({.channel_index = 2, .fragment_index = 4});

    XCTAssertEqual(called0.size(), 2);
    XCTAssertEqual(called0.at(1), 4);
    XCTAssertEqual(called1.size(), 2);

    buffering->overwrite_element_on_render({.channel_index = 0, .fragment_index = 5});
    buffering->overwrite_element_on_render({.channel_index = 1, .fragment_index = 6});

    XCTAssertEqual(called0.size(), 2);
    XCTAssertEqual(called1.size(), 2);
}

- (void)test_read_into_buffer {
    self->_cpp.setup_advancing();

    auto const &buffering = self->_cpp.buffering;
    auto &channels = self->_cpp.channels;

    bool result0 = false;
    bool result1 = false;
    std::vector<std::pair<audio::pcm_buffer *, frame_index_t>> called0;
    std::vector<std::pair<audio::pcm_buffer *, frame_index_t>> called1;

    channels.at(0)->read_into_buffer_handler = [&called0, &result0](audio::pcm_buffer *buffer,
                                                                    frame_index_t const frame) {
        called0.emplace_back(buffer, frame);
        return result0;
    };

    channels.at(1)->read_into_buffer_handler = [&called1, &result1](audio::pcm_buffer *buffer,
                                                                    frame_index_t const frame) {
        called1.emplace_back(buffer, frame);
        return result1;
    };

    audio::pcm_buffer buffer{test::format, test::sample_rate};

    result0 = false;
    result1 = false;

    XCTAssertFalse(buffering->read_into_buffer_on_render(&buffer, 0, 100));

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called0.at(0).first, &buffer);
    XCTAssertEqual(called0.at(0).second, 100);
    XCTAssertEqual(called1.size(), 0);

    XCTAssertFalse(buffering->read_into_buffer_on_render(&buffer, 1, 101));

    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called1.size(), 1);
    XCTAssertEqual(called1.at(0).first, &buffer);
    XCTAssertEqual(called1.at(0).second, 101);

    XCTAssertFalse(buffering->read_into_buffer_on_render(&buffer, 2, 102));

    // ch_idxが範囲外で呼ばれない
    XCTAssertEqual(called0.size(), 1);
    XCTAssertEqual(called1.size(), 1);

    result0 = true;
    result1 = false;

    XCTAssertTrue(buffering->read_into_buffer_on_render(&buffer, 0, 300), @"channelから返したフラグと一致");
    XCTAssertFalse(buffering->read_into_buffer_on_render(&buffer, 1, 301));
}

@end
