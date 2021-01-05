//
//  yas_playing_channel_mapping_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_channel_mapping_tests : XCTestCase

@end

@implementation yas_playing_channel_mapping_tests

- (void)test_make_empty {
    auto const mapping = channel_mapping::make_shared();

    XCTAssertEqual(mapping->indices.size(), 0);
}

- (void)test_make_with_indices {
    auto const mapping = channel_mapping::make_shared({3, 2, 1});

    XCTAssertEqual(mapping->indices, (std::vector<channel_index_t>{3, 2, 1}));
}

- (void)test_mapped {
    auto const mapping = channel_mapping::make_shared({3, 2, 1});

    XCTAssertEqual(mapping->mapped_index(-1, 3), std::nullopt);
    XCTAssertEqual(mapping->mapped_index(0, 3), 3);
    XCTAssertEqual(mapping->mapped_index(1, 3), 2);
    XCTAssertEqual(mapping->mapped_index(2, 3), 1);
    XCTAssertEqual(mapping->mapped_index(4, 3), std::nullopt);

    XCTAssertEqual(mapping->mapped_index(-1, 2), std::nullopt);
    XCTAssertEqual(mapping->mapped_index(0, 2), 3);
    XCTAssertEqual(mapping->mapped_index(1, 2), 2);
    XCTAssertEqual(mapping->mapped_index(2, 2), std::nullopt);
}

- (void)test_unmapped {
    auto const mapping = channel_mapping::make_shared({3, 2, 1});

    XCTAssertEqual(mapping->unmapped_index(-1, 3), std::nullopt);
    XCTAssertEqual(mapping->unmapped_index(0, 3), std::nullopt);
    XCTAssertEqual(mapping->unmapped_index(1, 3), 2);
    XCTAssertEqual(mapping->unmapped_index(2, 3), 1);
    XCTAssertEqual(mapping->unmapped_index(3, 3), 0);
    XCTAssertEqual(mapping->unmapped_index(4, 3), std::nullopt);

    XCTAssertEqual(mapping->unmapped_index(-1, 2), std::nullopt);
    XCTAssertEqual(mapping->unmapped_index(0, 2), std::nullopt);
    XCTAssertEqual(mapping->unmapped_index(1, 2), std::nullopt);
    XCTAssertEqual(mapping->unmapped_index(2, 2), 1);
    XCTAssertEqual(mapping->unmapped_index(3, 2), 0);
    XCTAssertEqual(mapping->unmapped_index(4, 2), std::nullopt);
}

@end
