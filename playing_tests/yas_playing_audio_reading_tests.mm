//
//  yas_playing_audio_reading_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>
#import <future>
#import <thread>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_audio_reading_tests : XCTestCase

@end

@implementation yas_playing_audio_reading_tests

- (void)test_initial_state {
    auto const reading = audio_reading::make_shared();

    XCTAssertEqual(reading->state(), audio_reading::state_t::initial);
}

- (void)test_reset_and_create_buffer {
    auto const reading = audio_reading::make_shared();

    std::promise<void> create_promise;
    std::promise<void> render_promise;

    double const sample_rate = 44100;
    auto const pcm_format = audio::pcm_format::float32;
    uint32_t const length = 2;

    std::thread task_thread{[&reading, &create_promise] {
        while (true) {
            if (reading->state() == audio_reading::state_t::creating) {
                reading->create_buffer_on_task();
                create_promise.set_value();
                break;
            }
            std::this_thread::yield();
        }
    }};

    std::thread render_thread{[&reading, &sample_rate, &pcm_format, &length, &render_promise] {
        render_promise.set_value();
        reading->set_creating_on_render(sample_rate, pcm_format, length);
    }};

    create_promise.get_future().get();
    render_promise.get_future().get();

    XCTAssertEqual(reading->state(), audio_reading::state_t::rendering);
    XCTAssertTrue(reading->buffer_on_render() != nullptr);
    XCTAssertEqual(reading->buffer_on_render()->format().sample_rate(), sample_rate);
    XCTAssertEqual(reading->buffer_on_render()->format().pcm_format(), pcm_format);
    XCTAssertEqual(reading->buffer_on_render()->format().channel_count(), 1);
    XCTAssertEqual(reading->buffer_on_render()->frame_length(), length);

    render_thread.join();
    task_thread.join();
}

- (void)test_needs_create_on_render {
    auto const reading = audio_reading::make_shared();

    std::thread{[&reading] { reading->set_creating_on_render(4, audio::pcm_format::int16, 2); }}.join();
    std::thread{[&reading] { reading->create_buffer_on_task(); }}.join();

    std::thread{[&reading] {
        XCTAssertFalse(reading->needs_create_on_render(4, audio::pcm_format::int16, 2), @"元と同じ");
        XCTAssertFalse(reading->needs_create_on_render(4, audio::pcm_format::int16, 1), @"lengthが小さくなった");

        XCTAssertTrue(reading->needs_create_on_render(5, audio::pcm_format::int16, 2), @"sample_rateが変わった");
        XCTAssertFalse(reading->needs_create_on_render(4, audio::pcm_format::float32, 2), @"pcm_formatが変わった");
        XCTAssertTrue(reading->needs_create_on_render(4, audio::pcm_format::int16, 3), @"lengthが大きくなった");
    }}.join();
}

@end
