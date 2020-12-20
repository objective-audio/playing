//
//  yas_playing_timeline_canceling_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_timeline_canceling_tests : XCTestCase

@end

@implementation yas_playing_timeline_canceling_tests

- (void)setUp {
}

- (void)tearDown {
}

- (void)test_range_cancel_request {
    auto matcher = timeline_cancel_matcher::make_shared(proc::time::range{1, 2});
    auto range_request_a = timeline_range_cancel_request::make_shared({1, 2});
    auto range_request_b = timeline_range_cancel_request::make_shared({0, 3});
    auto range_request_c = timeline_range_cancel_request::make_shared({1, 3});

    XCTAssertTrue(task_cancel_id::cast(matcher)->is_equal(range_request_a));
    XCTAssertTrue(task_cancel_id::cast(matcher)->is_equal(range_request_b));
    XCTAssertTrue(task_cancel_id::cast(matcher)->is_equal(range_request_c), );
    XCTAssertTrue(task_cancel_id::cast(range_request_a)->is_equal(matcher));
    XCTAssertTrue(task_cancel_id::cast(range_request_b)->is_equal(matcher));
    XCTAssertTrue(task_cancel_id::cast(range_request_c)->is_equal(matcher));

    auto range_request_d = timeline_range_cancel_request::make_shared({1, 1});
    auto range_request_e = timeline_range_cancel_request::make_shared({2, 1});
    auto range_request_f = timeline_range_cancel_request::make_shared({0, 2});
    auto range_request_g = timeline_range_cancel_request::make_shared({2, 2});

    XCTAssertFalse(task_cancel_id::cast(matcher)->is_equal(range_request_d));
    XCTAssertFalse(task_cancel_id::cast(matcher)->is_equal(range_request_e));
    XCTAssertFalse(task_cancel_id::cast(matcher)->is_equal(range_request_f));
    XCTAssertFalse(task_cancel_id::cast(matcher)->is_equal(range_request_g));
    XCTAssertFalse(task_cancel_id::cast(range_request_d)->is_equal(matcher));
    XCTAssertFalse(task_cancel_id::cast(range_request_e)->is_equal(matcher));
    XCTAssertFalse(task_cancel_id::cast(range_request_f)->is_equal(matcher));
    XCTAssertFalse(task_cancel_id::cast(range_request_g)->is_equal(matcher));
}

@end
