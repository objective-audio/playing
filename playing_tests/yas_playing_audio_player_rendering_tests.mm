//
//  yas_playing_audio_player_rendering_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>
#import "yas_playing_audio_player_test_utils.h"

using namespace yas;
using namespace yas::playing;

@interface yas_playing_audio_player_rendering_tests : XCTestCase

@end

@implementation yas_playing_audio_player_rendering_tests {
    test::audio_player_cpp _cpp;
}

- (void)test_pull_seek_frame {
    audio::pcm_buffer buffer = test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_buffering_rendering();

    auto const &buffering = self->_cpp.buffering;
    auto const &rendering = self->_cpp.resource;

    std::size_t called_reset_overwrite = 0;
    std::size_t called_pull_seek = 0;
    std::vector<frame_index_t> called_set_current_frame;
    std::vector<std::pair<frame_index_t, std::optional<channel_mapping_ptr>>> called_set_all_writing;
    std::size_t called_pull_ch_mapping = 0;

    std::optional<frame_index_t> seek_frame = 300;
    auto ch_mapping = channel_mapping::make_shared({13, 14, 15});

    rendering->reset_overwrite_requests_handler = [&called_reset_overwrite] { ++called_reset_overwrite; };
    rendering->pull_seek_frame_handler = [&seek_frame, &called_pull_seek] {
        ++called_pull_seek;
        return seek_frame;
    };
    rendering->set_current_frame_handler = [&called_set_current_frame](frame_index_t frame) {
        called_set_current_frame.emplace_back(frame);
    };
    buffering->set_all_writing_handler = [&called_set_all_writing](frame_index_t frame,
                                                                   std::optional<channel_mapping_ptr> &&ch_mapping) {
        called_set_all_writing.emplace_back(frame, ch_mapping);
    };
    rendering->pull_ch_mapping_handler = [&called_pull_ch_mapping, &ch_mapping] {
        ++called_pull_ch_mapping;
        return ch_mapping;
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_pull_seek, 1);
    XCTAssertEqual(called_reset_overwrite, 1);
    XCTAssertEqual(called_set_current_frame.size(), 1);
    XCTAssertEqual(called_set_current_frame.at(0), 300);
    XCTAssertEqual(called_pull_ch_mapping, 1);
    XCTAssertEqual(called_set_all_writing.size(), 1);
    XCTAssertEqual(called_set_all_writing.at(0).first, 300);
    XCTAssertEqual(called_set_all_writing.at(0).second.value()->indices, (std::vector<channel_index_t>{13, 14, 15}));
}

- (void)test_pull_ch_mapping {
    audio::pcm_buffer buffer = test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_buffering_rendering();

    auto const &buffering = self->_cpp.buffering;
    auto const &rendering = self->_cpp.resource;

    std::size_t called_reset_overwrite = 0;
    std::size_t called_pull_seek = 0;
    std::size_t called_current_frame = 0;
    std::vector<frame_index_t> called_set_current_frame;
    std::vector<std::pair<frame_index_t, std::optional<channel_mapping_ptr>>> called_set_all_writing;
    std::size_t called_pull_ch_mapping = 0;

    frame_index_t current_frame = 400;
    std::optional<frame_index_t> seek_frame = std::nullopt;
    std::optional<channel_mapping_ptr> ch_mapping = channel_mapping::make_shared({16, 17, 18});

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

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_pull_seek, 1);
    XCTAssertEqual(called_reset_overwrite, 1);
    XCTAssertEqual(called_current_frame, 1);
    XCTAssertEqual(called_pull_ch_mapping, 1);
    XCTAssertEqual(called_set_all_writing.size(), 1);
    XCTAssertEqual(called_set_all_writing.at(0).first, 400);
    XCTAssertEqual(called_set_all_writing.at(0).second.value()->indices, (std::vector<channel_index_t>{16, 17, 18}));
}

- (void)test_perform_overwrite_requests {
    audio::pcm_buffer buffer = test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_ch_mapping();

    auto const &rendering = self->_cpp.resource;

    std::vector<test::resource::overwrite_requests_f> called_perform;
    std::size_t called_is_playing = 0;

    rendering->perform_overwrite_requests_handler =
        [&called_perform](test::resource::overwrite_requests_f const &handler) {
            called_perform.emplace_back(handler);
        };

    rendering->is_playing_handler = [&called_is_playing] {
        ++called_is_playing;
        return false;
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_perform.size(), 1);
    XCTAssertEqual(called_is_playing, 1);
}

- (void)test_is_playing {
    audio::pcm_buffer buffer = test::audio_player_cpp::make_out_buffer();

    self->_cpp.skip_ch_mapping();

    auto const &rendering = self->_cpp.resource;
    auto const &reading = self->_cpp.reading;

    bool is_playing = false;
    std::size_t called_is_playing = 0;
    std::size_t called_buffer = 0;

    rendering->perform_overwrite_requests_handler = [](test::resource::overwrite_requests_f const &) {};

    rendering->is_playing_handler = [&is_playing, &called_is_playing] {
        ++called_is_playing;
        return is_playing;
    };

    reading->buffer_handler = [&called_buffer] {
        ++called_buffer;
        return nullptr;
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_is_playing, 1);
    XCTAssertEqual(called_buffer, 0);

    is_playing = true;

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_is_playing, 2);
    XCTAssertEqual(called_buffer, 1);
}

- (void)test_rendering {
    auto buffer = self->_cpp.make_out_buffer();

    self->_cpp.skip_playing();

    auto const &resource = self->_cpp.resource;
    auto const &buffering = self->_cpp.buffering;

    frame_index_t current_frame = 0;
    std::vector<fragment_index_t> called_advance;
    std::vector<frame_index_t> called_set_current_frame;
    std::vector<std::pair<channel_index_t, frame_index_t>> called_read_into;

    resource->current_frame_handler = [&current_frame] { return current_frame; };
    buffering->fragment_length_handler = [] { return 4; };
    buffering->channel_count_handler = [] { return 3; };
    buffering->read_into_buffer_handler = [&called_read_into](audio::pcm_buffer *buffer, channel_index_t ch_idx,
                                                              frame_index_t frame_idx) {
        test::audio_player_cpp::fill_buffer(buffer, ch_idx, frame_idx);
        called_read_into.emplace_back(ch_idx, frame_idx);
        return true;
    };
    buffering->advance_handler = [&called_advance](fragment_index_t frag_idx) {
        called_advance.emplace_back(frag_idx);
    };
    resource->set_current_frame_handler = [&called_set_current_frame](frame_index_t frame) {
        called_set_current_frame.emplace_back(frame);
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_advance.size(), 0);
    XCTAssertEqual(called_set_current_frame.size(), 1);
    XCTAssertEqual(called_set_current_frame.at(0), 2);
    XCTAssertEqual(called_read_into.size(), 3);
    XCTAssertEqual(called_read_into.at(0).first, 0);
    XCTAssertEqual(called_read_into.at(0).second, 0);
    XCTAssertEqual(called_read_into.at(1).first, 1);
    XCTAssertEqual(called_read_into.at(1).second, 0);
    XCTAssertEqual(called_read_into.at(2).first, 2);
    XCTAssertEqual(called_read_into.at(2).second, 0);

    auto const *data0 = buffer.data_ptr_at_index<int16_t>(0);
    XCTAssertEqual(data0[0], 0);
    XCTAssertEqual(data0[1], 1);
    auto const *data1 = buffer.data_ptr_at_index<int16_t>(1);
    XCTAssertEqual(data1[0], 1000);
    XCTAssertEqual(data1[1], 1001);
    auto const *data2 = buffer.data_ptr_at_index<int16_t>(2);
    XCTAssertEqual(data2[0], 2000);
    XCTAssertEqual(data2[1], 2001);
}

- (void)test_rendering_advance {
    auto buffer = self->_cpp.make_out_buffer();

    self->_cpp.skip_playing();

    auto const &resource = self->_cpp.resource;
    auto const &buffering = self->_cpp.buffering;

    frame_index_t current_frame = 10;
    std::vector<fragment_index_t> called_advance;
    std::vector<frame_index_t> called_set_current_frame;
    std::vector<std::pair<channel_index_t, frame_index_t>> called_read_into;

    resource->current_frame_handler = [&current_frame] { return current_frame; };
    buffering->fragment_length_handler = [] { return 1; };
    buffering->channel_count_handler = [] { return 3; };
    buffering->read_into_buffer_handler = [&called_read_into](audio::pcm_buffer *buffer, channel_index_t ch_idx,
                                                              frame_index_t frame_idx) {
        test::audio_player_cpp::fill_buffer(buffer, ch_idx, frame_idx);
        called_read_into.emplace_back(ch_idx, frame_idx);
        return true;
    };
    buffering->advance_handler = [&called_advance](fragment_index_t frag_idx) {
        called_advance.emplace_back(frag_idx);
    };
    resource->set_current_frame_handler = [&called_set_current_frame](frame_index_t frame) {
        called_set_current_frame.emplace_back(frame);
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_advance.size(), 2);
    XCTAssertEqual(called_advance.at(0), 11);
    XCTAssertEqual(called_advance.at(1), 12);
    XCTAssertEqual(called_set_current_frame.size(), 2);
    XCTAssertEqual(called_set_current_frame.at(0), 11);
    XCTAssertEqual(called_set_current_frame.at(1), 12);

    XCTAssertEqual(called_read_into.size(), 6);
    XCTAssertEqual(called_read_into.at(0).first, 0);
    XCTAssertEqual(called_read_into.at(0).second, 10);
    XCTAssertEqual(called_read_into.at(1).first, 1);
    XCTAssertEqual(called_read_into.at(1).second, 10);
    XCTAssertEqual(called_read_into.at(2).first, 2);
    XCTAssertEqual(called_read_into.at(2).second, 10);
    XCTAssertEqual(called_read_into.at(3).first, 0);
    XCTAssertEqual(called_read_into.at(3).second, 11);
    XCTAssertEqual(called_read_into.at(4).first, 1);
    XCTAssertEqual(called_read_into.at(4).second, 11);
    XCTAssertEqual(called_read_into.at(5).first, 2);
    XCTAssertEqual(called_read_into.at(5).second, 11);

    auto const *data0 = buffer.data_ptr_at_index<int16_t>(0);
    XCTAssertEqual(data0[0], 10);
    XCTAssertEqual(data0[1], 11);
    auto const *data1 = buffer.data_ptr_at_index<int16_t>(1);
    XCTAssertEqual(data1[0], 1010);
    XCTAssertEqual(data1[1], 1011);
    auto const *data2 = buffer.data_ptr_at_index<int16_t>(2);
    XCTAssertEqual(data2[0], 2010);
    XCTAssertEqual(data2[1], 2011);
}

- (void)test_rendering_less_channel {
    auto buffer = self->_cpp.make_out_buffer();

    self->_cpp.skip_playing();

    auto const &resource = self->_cpp.resource;
    auto const &buffering = self->_cpp.buffering;

    frame_index_t current_frame = 20;
    std::vector<fragment_index_t> called_advance;
    std::vector<frame_index_t> called_set_current_frame;
    std::vector<std::pair<channel_index_t, frame_index_t>> called_read_into;

    resource->current_frame_handler = [&current_frame] { return current_frame; };
    buffering->fragment_length_handler = [] { return 4; };
    buffering->channel_count_handler = [] { return 1; };
    buffering->read_into_buffer_handler = [&called_read_into](audio::pcm_buffer *buffer, channel_index_t ch_idx,
                                                              frame_index_t frame_idx) {
        test::audio_player_cpp::fill_buffer(buffer, ch_idx, frame_idx);
        called_read_into.emplace_back(ch_idx, frame_idx);
        return true;
    };
    buffering->advance_handler = [&called_advance](fragment_index_t frag_idx) {
        called_advance.emplace_back(frag_idx);
    };
    resource->set_current_frame_handler = [&called_set_current_frame](frame_index_t frame) {
        called_set_current_frame.emplace_back(frame);
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_advance.size(), 0);
    XCTAssertEqual(called_set_current_frame.size(), 1);
    XCTAssertEqual(called_set_current_frame.at(0), 22);
    XCTAssertEqual(called_read_into.size(), 1);
    XCTAssertEqual(called_read_into.at(0).first, 0);
    XCTAssertEqual(called_read_into.at(0).second, 20);

    auto const *data0 = buffer.data_ptr_at_index<int16_t>(0);
    XCTAssertEqual(data0[0], 20);
    XCTAssertEqual(data0[1], 21);
    auto const *data1 = buffer.data_ptr_at_index<int16_t>(1);
    XCTAssertEqual(data1[0], 0);
    XCTAssertEqual(data1[1], 0);
    auto const *data2 = buffer.data_ptr_at_index<int16_t>(2);
    XCTAssertEqual(data2[0], 0);
    XCTAssertEqual(data2[1], 0);
}

- (void)test_rendering_read_failed {
    auto buffer = self->_cpp.make_out_buffer();

    self->_cpp.skip_playing();

    auto const &resource = self->_cpp.resource;
    auto const &buffering = self->_cpp.buffering;

    frame_index_t current_frame = 30;
    std::vector<fragment_index_t> called_advance;
    std::vector<frame_index_t> called_set_current_frame;
    std::vector<std::pair<channel_index_t, frame_index_t>> called_read_into;

    resource->current_frame_handler = [&current_frame] { return current_frame; };
    buffering->fragment_length_handler = [] { return 1; };
    buffering->channel_count_handler = [] { return 3; };
    buffering->read_into_buffer_handler = [&called_read_into](audio::pcm_buffer *buffer, channel_index_t ch_idx,
                                                              frame_index_t frame_idx) {
        called_read_into.emplace_back(ch_idx, frame_idx);

        if (frame_idx != 30) {
            return false;
        }

        test::audio_player_cpp::fill_buffer(buffer, ch_idx, frame_idx);
        return true;
    };
    buffering->advance_handler = [&called_advance](fragment_index_t frag_idx) {
        called_advance.emplace_back(frag_idx);
    };
    resource->set_current_frame_handler = [&called_set_current_frame](frame_index_t frame) {
        called_set_current_frame.emplace_back(frame);
    };

    self->_cpp.rendering_handler(&buffer);

    XCTAssertEqual(called_advance.size(), 1);
    XCTAssertEqual(called_advance.at(0), 31);
    XCTAssertEqual(called_set_current_frame.size(), 1);
    XCTAssertEqual(called_set_current_frame.at(0), 31);
    XCTAssertEqual(called_read_into.size(), 4);
    XCTAssertEqual(called_read_into.at(0).first, 0);
    XCTAssertEqual(called_read_into.at(0).second, 30);
    XCTAssertEqual(called_read_into.at(1).first, 1);
    XCTAssertEqual(called_read_into.at(1).second, 30);
    XCTAssertEqual(called_read_into.at(2).first, 2);
    XCTAssertEqual(called_read_into.at(2).second, 30);
    XCTAssertEqual(called_read_into.at(3).first, 0);
    XCTAssertEqual(called_read_into.at(3).second, 31);

    auto const *data0 = buffer.data_ptr_at_index<int16_t>(0);
    XCTAssertEqual(data0[0], 30);
    XCTAssertEqual(data0[1], 0);
    auto const *data1 = buffer.data_ptr_at_index<int16_t>(1);
    XCTAssertEqual(data1[0], 1030);
    XCTAssertEqual(data1[1], 0);
    auto const *data2 = buffer.data_ptr_at_index<int16_t>(2);
    XCTAssertEqual(data2[0], 2030);
    XCTAssertEqual(data2[1], 0);
}

@end
