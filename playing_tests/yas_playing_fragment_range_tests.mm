//
//  yas_playing_fragment_range_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/yas_playing_umbrella.hpp>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_fragment_range_tests : XCTestCase

@end

@implementation yas_playing_fragment_range_tests

- (void)test_contains {
    XCTAssertFalse((fragment_range{.index = 0, .length = 1}.contains(-1)));
    XCTAssertTrue((fragment_range{.index = 0, .length = 1}.contains(0)));
    XCTAssertFalse((fragment_range{.index = 0, .length = 1}.contains(1)));
    XCTAssertFalse((fragment_range{.index = 0, .length = 1}.contains(2)));

    XCTAssertFalse((fragment_range{.index = -1, .length = 2}.contains(-2)));
    XCTAssertTrue((fragment_range{.index = -1, .length = 2}.contains(-1)));
    XCTAssertTrue((fragment_range{.index = -1, .length = 2}.contains(0)));
    XCTAssertFalse((fragment_range{.index = -1, .length = 2}.contains(1)));
}

- (void)test_equal {
    XCTAssertTrue((fragment_range{.index = 0, .length = 1}) == (fragment_range{.index = 0, .length = 1}));
    XCTAssertFalse((fragment_range{.index = 0, .length = 1}) == (fragment_range{.index = 0, .length = 2}));
    XCTAssertFalse((fragment_range{.index = 0, .length = 1}) == (fragment_range{.index = 3, .length = 1}));

    XCTAssertFalse((fragment_range{.index = 0, .length = 1}) != (fragment_range{.index = 0, .length = 1}));
    XCTAssertTrue((fragment_range{.index = 0, .length = 1}) != (fragment_range{.index = 0, .length = 2}));
    XCTAssertTrue((fragment_range{.index = 0, .length = 1}) != (fragment_range{.index = 3, .length = 1}));
}

@end
