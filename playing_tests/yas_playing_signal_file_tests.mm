//
//  yas_playing_signal_file_tests.mm
//

#import <XCTest/XCTest.h>
#import <audio/yas_audio_format.h>
#import <audio/yas_audio_pcm_buffer.h>
#import <cpp_utils/yas_file_manager.h>
#import <cpp_utils/yas_file_path.h>
#import <cpp_utils/yas_system_path_utils.h>
#import <playing/yas_playing_signal_file.h>
#import <processing/yas_processing_signal_event.h>
#import "yas_playing_test_utils.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::signal_file_test {
struct cpp {
    std::string const root_path = test_utils::root_path();
};
}

@interface yas_playing_signal_file_tests : XCTestCase

@end

@implementation yas_playing_signal_file_tests {
    signal_file_test::cpp _cpp;
}

- (void)setUp {
    file_manager::remove_content(self->_cpp.root_path);
}

- (void)tearDown {
    file_manager::remove_content(self->_cpp.root_path);
}

- (void)test_read_with_data_ptr {
    auto dir_result = file_manager::create_directory_if_not_exists(self->_cpp.root_path);

    XCTAssertTrue(dir_result);

    auto const path = file_path{self->_cpp.root_path}.appending("signal").string();

    auto write_event = proc::make_signal_event<int64_t>(2);
    write_event.data<int64_t>()[0] = 10;
    write_event.data<int64_t>()[1] = 11;

    auto const write_result = signal_file::write(path, write_event);

    XCTAssertTrue(write_result);

    int64_t read_data[2];

    auto const read_result = signal_file::read(path, read_data, sizeof(int64_t) * 2);

    XCTAssertTrue(read_result);

    XCTAssertEqual(read_data[0], 10);
    XCTAssertEqual(read_data[1], 11);
}

- (void)test_read_with_buffer {
    auto dir_result = file_manager::create_directory_if_not_exists(self->_cpp.root_path);

    XCTAssertTrue(dir_result);

    auto const path = file_path{self->_cpp.root_path}.appending("signal").string();
    signal_file_info const file_info{path, proc::time::range{0, 2}, typeid(double)};

    auto write_event = proc::make_signal_event<double>(2);
    write_event.data<double>()[0] = 1.0;
    write_event.data<double>()[1] = 2.0;

    auto const write_result = signal_file::write(path, write_event);

    XCTAssertTrue(write_result);

    audio::format const format{
        {.sample_rate = 2.0, .channel_count = 1, .pcm_format = audio::pcm_format::float64, .interleaved = false}};
    audio::pcm_buffer buffer{format, 2};

    auto const read_result = signal_file::read(file_info, buffer, 0);

    XCTAssertTrue(read_result);

    XCTAssertEqual(buffer.data_ptr_at_index<double>(0)[0], 1.0);
    XCTAssertEqual(buffer.data_ptr_at_index<double>(0)[1], 2.0);
}

- (void)test_write_error_to_string {
    XCTAssertEqual(to_string(playing::signal_file::write_error::open_stream_failed), "open_stream_failed");
    XCTAssertEqual(to_string(playing::signal_file::write_error::write_to_stream_failed), "write_to_stream_failed");
    XCTAssertEqual(to_string(playing::signal_file::write_error::close_stream_failed), "close_stream_failed");
}

- (void)test_read_error_to_string {
    XCTAssertEqual(to_string(playing::signal_file::read_error::invalid_sample_type), "invalid_sample_type");
    XCTAssertEqual(to_string(playing::signal_file::read_error::out_of_range), "out_of_range");
    XCTAssertEqual(to_string(playing::signal_file::read_error::open_stream_failed), "open_stream_failed");
    XCTAssertEqual(to_string(playing::signal_file::read_error::read_from_stream_failed), "read_from_stream_failed");
    XCTAssertEqual(to_string(playing::signal_file::read_error::read_count_not_match), "read_count_not_match");
    XCTAssertEqual(to_string(playing::signal_file::read_error::close_stream_failed), "close_stream_failed");
}

@end
