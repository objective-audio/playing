//
//  yas_audio_circular_buffer_chain_tests.mm
//

#import <XCTest/XCTest.h>
#import <cpp_utils/cpp_utils.h>
#import <playing/yas_playing_audio_circular_buffer.h>
#import "yas_playing_test_utils.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::audio_circular_buffer_chain_test {
struct cpp {
    task_queue queue{nullptr};
    sample_rate_t const sample_rate = 3;
    channel_index_t const ch_idx = 0;
    std::size_t const ch_count = 1;
    audio::format const format{audio::format::args{.sample_rate = static_cast<double>(this->sample_rate),
                                                   .channel_count = 1,
                                                   .pcm_format = audio::pcm_format::int16,
                                                   .interleaved = false}};
};
}

@interface yas_audio_circular_buffer_chain_tests : XCTestCase

@end

@implementation yas_audio_circular_buffer_chain_tests {
    audio_circular_buffer_chain_test::cpp _cpp;
}

- (void)setUp {
    file_manager::remove_content(test_utils::root_path());

    self->_cpp.queue = task_queue{};
}

- (void)tearDown {
    self->_cpp.queue = nullptr;

    file_manager::remove_content(test_utils::root_path());
}

- (void)test_states_chain {
    auto &cpp = self->_cpp;
    task_queue &queue = cpp.queue;

    auto circular_buffer =
        make_audio_circular_buffer(cpp.format, 2, queue, 0, [](audio::pcm_buffer &buffer, int64_t const frag_idx) {
            int64_t const top_frame_idx = frag_idx * 3;
            int16_t *data_ptr = buffer.data_ptr_at_index<int16_t>(0);
            data_ptr[0] = top_frame_idx;
            data_ptr[1] = top_frame_idx + 1;
            data_ptr[2] = top_frame_idx + 2;
            return true;
        });

    chaining::observer_pool pool;

    playing::audio_circular_buffer::state_map_holder_t states;

    pool += circular_buffer->states_chain().send_to(states.receiver()).sync();

    XCTAssertEqual(states.size(), 0);

    std::vector<chaining::event> received;

    pool +=
        circular_buffer->states_chain().perform([&received](auto const &event) { received.push_back(event); }).sync();

    XCTAssertEqual(received.size(), 1);
    XCTAssertEqual(received.at(0).type(), chaining::event_type::fetched);

    circular_buffer->reload_all_buffers(-1);

    queue.wait_until_all_tasks_are_finished();

    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]];

    XCTAssertEqual(received.size(), 5);
    XCTAssertEqual(received.at(1).type(), chaining::event_type::inserted);
    // 間の2つの順番は不定
    //    XCTAssertEqual(received.at(2).type(), chaining::event_type::inserted);
    //    XCTAssertEqual(received.at(3).type(), chaining::event_type::replaced);
    XCTAssertEqual(received.at(4).type(), chaining::event_type::replaced);

    XCTAssertEqual(states.size(), 2);
    XCTAssertTrue(states.has_value(0));
    XCTAssertEqual(states.at(0).frag_idx, -1);
    XCTAssertEqual(states.at(0).kind, audio_buffer::state_kind::loaded);
    XCTAssertTrue(states.has_value(1));
    XCTAssertEqual(states.at(1).frag_idx, 0);
    XCTAssertEqual(states.at(1).kind, audio_buffer::state_kind::loaded);
}

@end
