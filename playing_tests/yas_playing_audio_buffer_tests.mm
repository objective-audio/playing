//
//  yas_audio_buffer_tests.mm
//

#import <XCTest/XCTest.h>
#import <audio/audio.h>
#import <cpp_utils/cpp_utils.h>
#import <playing/playing.h>
#import "yas_playing_test_utils.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::audio_buffer_test {
static audio::format make_format(double sample_rate) {
    return audio::format{audio::format::args{
        .sample_rate = sample_rate, .channel_count = 1, .pcm_format = audio::pcm_format::int16, .interleaved = false}};
}

static audio_buffer::ptr make_container(audio::format format, uint32_t const file_length) {
    audio::pcm_buffer container_buffer{format, file_length};
    return make_audio_buffer(std::move(container_buffer), [](auto const &, auto const &) {});
}

static audio_buffer::ptr make_container(uint32_t const file_length) {
    auto format = make_format(file_length);
    return make_container(format, file_length);
}
}

@interface yas_audio_buffer_tests : XCTestCase

@end

@implementation yas_audio_buffer_tests

- (void)setUp {
}

- (void)tearDown {
}

- (void)test_initial {
    uint32_t const file_length = 3;
    auto format = audio_buffer_test::make_format(file_length);
    auto const container = playing::audio_buffer_test::make_container(format, file_length);

    XCTAssertTrue(container);
    XCTAssertFalse(container->fragment_idx());
    XCTAssertFalse(container->begin_frame());
    XCTAssertEqual(container->format(), format);

    audio::pcm_buffer reading_buffer{format, file_length};
    XCTAssertEqual(container->read_into_buffer(reading_buffer, 0).error(), audio_buffer::read_error::unloaded);
}

- (void)test_prepare_loading {
    auto const container = audio_buffer_test::make_container(3);

    XCTAssertFalse(container->fragment_idx());

    container->prepare_loading(0);

    XCTAssertEqual(*container->fragment_idx(), 0);
    XCTAssertEqual(*container->begin_frame(), 0);

    container->prepare_loading(1);

    XCTAssertEqual(*container->fragment_idx(), 1);
    XCTAssertEqual(*container->begin_frame(), 3);

    container->prepare_loading(-1);

    XCTAssertEqual(*container->fragment_idx(), -1);
    XCTAssertEqual(*container->begin_frame(), -3);
}

- (void)test_load_and_read_into_buffer {
    uint32_t const file_length = 3;
    auto format = audio_buffer_test::make_format(file_length);
    auto container = audio_buffer_test::make_container(format, file_length);

    container->prepare_loading(0);

    auto load_result = container->load(0, [self](audio::pcm_buffer &buffer, int64_t const frag_idx) {
        XCTAssertEqual(frag_idx, 0);

        int16_t *data_ptr = buffer.data_ptr_at_index<int16_t>(0);
        data_ptr[0] = 10;
        data_ptr[1] = 11;
        data_ptr[2] = 12;

        return true;
    });

    XCTAssertTrue(load_result);

    audio::pcm_buffer reading_buffer{format, file_length};

    auto read_result = container->read_into_buffer(reading_buffer, 0);

    XCTAssertTrue(read_result);

    int16_t const *data_ptr = reading_buffer.data_ptr_at_index<int16_t>(0);

    XCTAssertEqual(data_ptr[0], 10);
    XCTAssertEqual(data_ptr[1], 11);
    XCTAssertEqual(data_ptr[2], 12);
}

- (void)test_load_error {
    uint32_t const file_length = 3;
    auto format = audio_buffer_test::make_format(file_length);
    auto container = audio_buffer_test::make_container(format, file_length);

    container->prepare_loading(0);

    auto load_result = container->load(0, [self](audio::pcm_buffer &buffer, int64_t const frag_idx) { return false; });

    XCTAssertFalse(load_result);
}

@end
