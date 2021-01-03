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

- (void)setUp {
}

- (void)tearDown {
}

- (void)test_processing_info {
    {
        // ファイルの終わりに隣接しない
        auto const proc_length = player_utils::process_length(0, 2, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(0, proc_length, 3);

        XCTAssertEqual(proc_length, 2);
        XCTAssertFalse(adv_frag_idx);
    }

    {
        // ファイルの終わりに隣接する（1個目の前）
        auto const proc_length = player_utils::process_length(1, 3, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(1, proc_length, 3);

        XCTAssertEqual(proc_length, 2);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), 0);
    }

    {
        // ファイルの終わりに隣接する（2個目の前）
        auto const proc_length = player_utils::process_length(3, 6, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(3, proc_length, 3);

        XCTAssertEqual(proc_length, 3);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), 1);
    }

    {
        // ファイルの終わりに隣接する（0個目の前）
        auto const proc_length = player_utils::process_length(-3, 0, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(-3, proc_length, 3);

        XCTAssertEqual(proc_length, 3);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), -1);
    }

    {
        // ファイルの終わりに隣接する（-1個目の前）
        auto const proc_length = player_utils::process_length(-6, -3, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(-6, proc_length, 3);

        XCTAssertEqual(proc_length, 3);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), -2);
    }

    {
        // プラスのファイルの境界をまたぐ
        auto const proc_length = player_utils::process_length(2, 4, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(2, proc_length, 3);

        XCTAssertEqual(proc_length, 1);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), 0);
    }

    {
        // 0のファイルの境界をまたぐ
        auto const proc_length = player_utils::process_length(-1, 1, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(-1, proc_length, 3);

        XCTAssertEqual(proc_length, 1);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), -1);
    }

    {
        // マイナスのファイルの境界をまたぐ
        auto const proc_length = player_utils::process_length(-4, -2, 3);
        auto const adv_frag_idx = player_utils::advancing_fragment_index(-4, proc_length, 3);

        XCTAssertEqual(proc_length, 1);
        XCTAssertTrue(adv_frag_idx);
        XCTAssertEqual(adv_frag_idx.value(), -2);
    }
}

@end
