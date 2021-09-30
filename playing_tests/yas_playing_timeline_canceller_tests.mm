//
//  yas_playing_timeline_canceller_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_timeline_canceller_tests : XCTestCase

@end

@implementation yas_playing_timeline_canceller_tests

- (void)setUp {
}

- (void)tearDown {
}

- (void)test_cancel_by_range {
    auto matcher = timeline_canceller::make_shared(proc::time::range{1, 2});

    XCTAssertTrue(matcher->is_cancel({1, 2}));
    XCTAssertTrue(matcher->is_cancel({0, 3}));
    XCTAssertTrue(matcher->is_cancel({1, 3}));

    XCTAssertFalse(matcher->is_cancel({1, 1}));
    XCTAssertFalse(matcher->is_cancel({2, 1}));
    XCTAssertFalse(matcher->is_cancel({0, 2}));
    XCTAssertFalse(matcher->is_cancel({2, 2}));
}

@end
