//
//  yas_playing_numbers_file_tests.mm
//

#import <XCTest/XCTest.h>
#import <cpp_utils/yas_boolean.h>
#import <cpp_utils/yas_file_manager.h>
#import <cpp_utils/yas_file_path.h>
#import <cpp_utils/yas_system_path_utils.h>
#import <playing/yas_playing_numbers_file.h>
#import "yas_playing_test_utils.h"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::numbers_file_test {
struct cpp {
    std::string const root_path = test_utils::root_path();
};
}

@interface yas_playing_numbers_file_tests : XCTestCase

@end

@implementation yas_playing_numbers_file_tests {
    numbers_file_test::cpp _cpp;
}

- (void)setUp {
    file_manager::remove_content(self->_cpp.root_path);
}

- (void)tearDown {
    file_manager::remove_content(self->_cpp.root_path);
}

- (void)test_numbers_file {
    auto dir_result = file_manager::create_directory_if_not_exists(self->_cpp.root_path);

    XCTAssertTrue(dir_result);

    auto const path = file_path{self->_cpp.root_path}.appending("numbers").string();

    numbers_file::event_map_t write_events{
        {0, proc::make_number_event(double(0.0))},   {1, proc::make_number_event(float(1.0))},
        {2, proc::make_number_event(int64_t(2))},    {3, proc::make_number_event(uint64_t(3))},
        {4, proc::make_number_event(int32_t(4))},    {5, proc::make_number_event(uint32_t(5))},
        {6, proc::make_number_event(int16_t(6))},    {7, proc::make_number_event(uint16_t(7))},
        {8, proc::make_number_event(int8_t(8))},     {9, proc::make_number_event(uint8_t(9))},
        {10, proc::make_number_event(boolean(true))}};

    auto write_result = numbers_file::write(path, write_events);

    XCTAssertTrue(write_result);

    auto read_result = numbers_file::read(path);

    XCTAssertTrue(read_result);

    numbers_file::event_map_t const &read_events = read_result.value();

    XCTAssertEqual(read_events.size(), 11);

    XCTAssertEqual(read_events.find(0)->second, proc::make_number_event(double(0.0)));
    XCTAssertEqual(read_events.find(1)->second, proc::make_number_event(float(1.0)));
    XCTAssertEqual(read_events.find(2)->second, proc::make_number_event(int64_t(2)));
    XCTAssertEqual(read_events.find(3)->second, proc::make_number_event(uint64_t(3)));
    XCTAssertEqual(read_events.find(4)->second, proc::make_number_event(int32_t(4)));
    XCTAssertEqual(read_events.find(5)->second, proc::make_number_event(uint32_t(5)));
    XCTAssertEqual(read_events.find(6)->second, proc::make_number_event(int16_t(6)));
    XCTAssertEqual(read_events.find(7)->second, proc::make_number_event(uint16_t(7)));
    XCTAssertEqual(read_events.find(8)->second, proc::make_number_event(int8_t(8)));
    XCTAssertEqual(read_events.find(9)->second, proc::make_number_event(uint8_t(9)));
    XCTAssertEqual(read_events.find(10)->second, proc::make_number_event(boolean(true)));
}

- (void)test_write_error_to_string {
    XCTAssertEqual(to_string(playing::numbers_file::write_error::open_stream_failed), "open_stream_failed");
    XCTAssertEqual(to_string(playing::numbers_file::write_error::write_to_stream_failed), "write_to_stream_failed");
    XCTAssertEqual(to_string(playing::numbers_file::write_error::close_stream_failed), "close_stream_failed");
}

- (void)test_read_error_to_string {
    XCTAssertEqual(to_string(playing::numbers_file::read_error::open_stream_failed), "open_stream_failed");
    XCTAssertEqual(to_string(playing::numbers_file::read_error::read_frame_failed), "read_frame_failed");
    XCTAssertEqual(to_string(playing::numbers_file::read_error::read_sample_store_type_failed),
                   "read_sample_store_type_failed");
    XCTAssertEqual(to_string(playing::numbers_file::read_error::read_value_failed), "read_value_failed");
    XCTAssertEqual(to_string(playing::numbers_file::read_error::sample_store_type_not_found),
                   "sample_store_type_not_found");
}

@end