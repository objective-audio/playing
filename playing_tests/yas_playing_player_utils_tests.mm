//
//  yas_playing_audio_utils_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/yas_playing_player_utils.h>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_rendering_info_tests : XCTestCase

@end

@implementation yas_playing_rendering_info_tests

- (void)test_process_length {
    // ファイルの終わりに隣接しない
    XCTAssertEqual(player_utils::process_length(0, 2, 3), 2);
    // ファイルの終わりに隣接する（1個目の前）
    XCTAssertEqual(player_utils::process_length(1, 3, 3), 2);
    // ファイルの終わりに隣接する（2個目の前）
    XCTAssertEqual(player_utils::process_length(3, 6, 3), 3);
    // ファイルの終わりに隣接する（0個目の前）
    XCTAssertEqual(player_utils::process_length(-3, 0, 3), 3);
    // ファイルの終わりに隣接する（-1個目の前）
    XCTAssertEqual(player_utils::process_length(-6, -3, 3), 3);
    // プラスのファイルの境界をまたぐ
    XCTAssertEqual(player_utils::process_length(2, 4, 3), 1);
    // 0のファイルの境界をまたぐ
    XCTAssertEqual(player_utils::process_length(-1, 1, 3), 1);
    // マイナスのファイルの境界をまたぐ
    XCTAssertEqual(player_utils::process_length(-4, -2, 3), 1);
}

- (void)test_advancing_fragment_index {
    // ファイルの終わりに隣接しない
    XCTAssertFalse(player_utils::advancing_fragment_index(0, 2, 3));
    // ファイルの終わりに隣接する（1個目の前）
    XCTAssertEqual(player_utils::advancing_fragment_index(1, 2, 3).value(), 0);
    // ファイルの終わりに隣接する（2個目の前）
    XCTAssertEqual(player_utils::advancing_fragment_index(3, 3, 3).value(), 1);
    // ファイルの終わりに隣接する（0個目の前）
    XCTAssertEqual(player_utils::advancing_fragment_index(-3, 3, 3).value(), -1);
    // ファイルの終わりに隣接する（-1個目の前）
    XCTAssertEqual(player_utils::advancing_fragment_index(-6, 3, 3).value(), -2);
    // プラスのファイルの境界をまたぐ
    XCTAssertEqual(player_utils::advancing_fragment_index(2, 1, 3).value(), 0);
    // 0のファイルの境界をまたぐ
    XCTAssertEqual(player_utils::advancing_fragment_index(-1, 1, 3).value(), -1);
    // マイナスのファイルの境界をまたぐ
    XCTAssertEqual(player_utils::advancing_fragment_index(-4, 1, 3).value(), -2);
}

@end
