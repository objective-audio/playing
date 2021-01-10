//
//  yas_playing_player_buffering_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>
#import "yas_playing_player_test_utils.h"

using namespace yas;
using namespace yas::playing;

@interface yas_playing_player_buffering_tests : XCTestCase

@end

@implementation yas_playing_player_buffering_tests {
    player_test::audio_player_cpp _cpp;
}

- (void)tearDown {
    self->_cpp.reset();
}

- (void)test_setup_state_initial {
    audio::pcm_buffer buffer = player_test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_reading();

    auto const &buffering = self->_cpp.buffering;

    std::vector<std::tuple<double, audio::pcm_format, uint32_t>> called_set_creating;

    buffering->setup_state_handler = [] { return audio_buffering_setup_state::initial; };
    buffering->set_creating_handler = [&called_set_creating](sample_rate_t sample_rate, audio::pcm_format pcm_format,
                                                             uint32_t ch_count) {
        called_set_creating.emplace_back(sample_rate, pcm_format, ch_count);
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_set_creating.size(), 1);
    XCTAssertEqual(std::get<0>(called_set_creating.at(0)), 4);
    XCTAssertEqual(std::get<1>(called_set_creating.at(0)), audio::pcm_format::int16);
    XCTAssertEqual(std::get<2>(called_set_creating.at(0)), 3);
}

- (void)test_setup_state_creating {
    audio::pcm_buffer buffer = player_test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_reading();

    auto const &buffering = self->_cpp.buffering;

    std::vector<std::tuple<double, audio::pcm_format, uint32_t>> called_set_creating;

    buffering->setup_state_handler = [] { return audio_buffering_setup_state::creating; };
    buffering->set_creating_handler = [&called_set_creating](sample_rate_t sample_rate, audio::pcm_format pcm_format,
                                                             uint32_t ch_count) {
        called_set_creating.emplace_back(sample_rate, pcm_format, ch_count);
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_set_creating.size(), 0);
}

- (void)test_setup_state_rendering {
    audio::pcm_buffer buffer = player_test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_reading();

    auto const &buffering = self->_cpp.buffering;

    bool needs_create = true;
    std::vector<std::tuple<double, audio::pcm_format, uint32_t>> called_set_creating;
    std::vector<std::tuple<double, audio::pcm_format, uint32_t>> called_needs_create;

    buffering->setup_state_handler = [] { return audio_buffering_setup_state::rendering; };
    buffering->set_creating_handler = [&called_set_creating](sample_rate_t sample_rate, audio::pcm_format pcm_format,
                                                             uint32_t ch_count) {
        called_set_creating.emplace_back(sample_rate, pcm_format, ch_count);
    };
    buffering->needs_create_handler = [&called_needs_create, &needs_create](
                                          sample_rate_t sample_rate, audio::pcm_format pcm_format, uint32_t ch_count) {
        called_needs_create.emplace_back(sample_rate, pcm_format, ch_count);
        return needs_create;
    };
    buffering->rendering_state_handler = [] { return audio_buffering_rendering_state::all_writing; };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_set_creating.size(), 1);

    needs_create = false;

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_set_creating.size(), 1);
}

- (void)test_rendering_state_waiting {
    audio::pcm_buffer buffer = player_test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_buffering_setup();

    auto const &buffering = self->_cpp.buffering;
    auto const &rendering = self->_cpp.resource;

    std::size_t called_reset_overwrite = 0;
    std::size_t called_pull_seek = 0;
    std::size_t called_current_frame = 0;
    std::vector<std::pair<frame_index_t, std::optional<channel_mapping_ptr>>> called_set_all_writing;
    std::size_t called_pull_ch_mapping = 0;

    frame_index_t current_frame = 100;
    std::optional<frame_index_t> seek_frame = std::nullopt;
    auto ch_mapping = channel_mapping::make_shared({10, 11, 12});

    buffering->rendering_state_handler = [] { return audio_buffering_rendering_state::waiting; };
    rendering->reset_overwrite_requests_handler = [&called_reset_overwrite] { ++called_reset_overwrite; };
    rendering->pull_seek_frame_handler = [&seek_frame, &called_pull_seek] {
        ++called_pull_seek;
        return seek_frame;
    };
    rendering->current_frame_handler = [&called_current_frame, &current_frame] {
        ++called_current_frame;
        return current_frame;
    };
    buffering->set_all_writing_handler = [&called_set_all_writing](frame_index_t frame,
                                                                   std::optional<channel_mapping_ptr> &&ch_mapping) {
        called_set_all_writing.emplace_back(frame, ch_mapping);
    };
    rendering->pull_ch_mapping_handler = [&called_pull_ch_mapping, &ch_mapping] {
        ++called_pull_ch_mapping;
        return ch_mapping;
    };

    // seek_frameなし

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_reset_overwrite, 1);
    XCTAssertEqual(called_pull_seek, 1);
    XCTAssertEqual(called_current_frame, 1);
    XCTAssertEqual(called_set_all_writing.size(), 1);
    XCTAssertEqual(called_set_all_writing.at(0).first, 100);
    XCTAssertEqual(called_set_all_writing.at(0).second.value()->indices, (std::vector<channel_index_t>{10, 11, 12}));
    XCTAssertEqual(called_pull_ch_mapping, 1);

    // seek_frameあり

    seek_frame = 200;

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_reset_overwrite, 2);
    XCTAssertEqual(called_pull_seek, 2);
    XCTAssertEqual(called_current_frame, 1);
    XCTAssertEqual(called_set_all_writing.size(), 2);
    XCTAssertEqual(called_set_all_writing.at(1).first, 200);
    XCTAssertEqual(called_set_all_writing.at(1).second.value()->indices, (std::vector<channel_index_t>{10, 11, 12}));
    XCTAssertEqual(called_pull_ch_mapping, 2);
}

- (void)test_rendering_state_all_writing {
    audio::pcm_buffer buffer = player_test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_buffering_setup();

    auto const &buffering = self->_cpp.buffering;
    auto const &rendering = self->_cpp.resource;

    bool called_pull_seek = false;

    buffering->rendering_state_handler = [] { return audio_buffering_rendering_state::all_writing; };
    rendering->pull_seek_frame_handler = [&called_pull_seek] {
        called_pull_seek = true;
        return std::nullopt;
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertFalse(called_pull_seek);
}

- (void)test_rendering_state_advancing {
    audio::pcm_buffer buffer = player_test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_buffering_setup();

    auto const &buffering = self->_cpp.buffering;
    auto const &rendering = self->_cpp.resource;

    std::size_t called_pull_seek = 0;

    buffering->rendering_state_handler = [] { return audio_buffering_rendering_state::advancing; };
    rendering->reset_overwrite_requests_handler = [] {};
    rendering->pull_seek_frame_handler = [&called_pull_seek] {
        ++called_pull_seek;
        return 0;
    };
    rendering->set_current_frame_handler = [](frame_index_t frame) {};
    buffering->set_all_writing_handler = [](frame_index_t frame, std::optional<channel_mapping_ptr> &&ch_mapping) {};
    rendering->pull_ch_mapping_handler = [] { return std::nullopt; };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_pull_seek, 1);
}

@end
