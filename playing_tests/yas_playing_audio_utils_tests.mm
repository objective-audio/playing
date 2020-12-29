//
//  yas_playing_utils_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/yas_playing_audio_utils.h>
#import <playing/yas_playing_buffering_channel.h>
#import <playing/yas_playing_buffering_element.h>

using namespace yas;
using namespace yas::playing;

namespace yas::playing::test {
static buffering_element_ptr cast_to_element(buffering_element_protocol_ptr const &protocol) {
    return std::dynamic_pointer_cast<buffering_element>(protocol);
}
}

@interface yas_playing_audio_utils_tests : XCTestCase

@end

@implementation yas_playing_audio_utils_tests

- (void)test_make_channels {
    audio::format const format{
        {.sample_rate = 4, .channel_count = 2, .pcm_format = audio::pcm_format::int16, .interleaved = false}};
    auto const channel = audio_utils::make_channel(3, format, 5);

    auto const &elements = channel->elements_for_test();
    XCTAssertEqual(elements.size(), 3);

    auto const casted_element0 = test::cast_to_element(elements.at(0));
    auto const &buffer0 = casted_element0->buffer_for_test();
    XCTAssertEqual(buffer0.frame_length(), 5);
    XCTAssertEqual(buffer0.format(), format);

    auto const casted_element1 = test::cast_to_element(elements.at(1));
    auto const &buffer1 = casted_element1->buffer_for_test();
    XCTAssertEqual(buffer1.frame_length(), 5);
    XCTAssertEqual(buffer1.format(), format);

    auto const casted_element2 = test::cast_to_element(elements.at(2));
    auto const &buffer2 = casted_element2->buffer_for_test();
    XCTAssertEqual(buffer2.frame_length(), 5);
    XCTAssertEqual(buffer2.format(), format);
}

@end
