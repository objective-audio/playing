//
//  yas_playing_audio_utils_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

@interface yas_playing_audio_rendering_info_tests : XCTestCase

@end

@implementation yas_playing_audio_rendering_info_tests

- (void)setUp {
}

- (void)tearDown {
}

- (void)test_processing_info_1 {
    {
        // ファイルの終わりに隣接しない
        audio_rendering_info info{0, 2, 3};

        XCTAssertEqual(info.length, 2);
        XCTAssertFalse(info.next_frag_idx);
    }

    {
        // ファイルの終わりに隣接する（1個目の前）
        audio_rendering_info info{1, 3, 3};

        XCTAssertEqual(info.length, 2);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, 1);
    }

    {
        // ファイルの終わりに隣接する（2個目の前）
        audio_rendering_info info{3, 6, 3};

        XCTAssertEqual(info.length, 3);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, 2);
    }

    {
        // ファイルの終わりに隣接する（0個目の前）
        audio_rendering_info info{-3, 0, 3};

        XCTAssertEqual(info.length, 3);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, 0);
    }

    {
        // ファイルの終わりに隣接する（-1個目の前）
        audio_rendering_info info{-6, -3, 3};

        XCTAssertEqual(info.length, 3);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, -1);
    }

    {
        // プラスのファイルの境界をまたぐ
        audio_rendering_info info{2, 4, 3};

        XCTAssertEqual(info.length, 1);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, 1);
    }

    {
        // 0のファイルの境界をまたぐ
        audio_rendering_info info{-1, 1, 3};

        XCTAssertEqual(info.length, 1);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, 0);
    }

    {
        // マイナスのファイルの境界をまたぐ
        audio_rendering_info info{-4, -2, 3};

        XCTAssertEqual(info.length, 1);
        XCTAssertTrue(info.next_frag_idx);
        XCTAssertEqual(*info.next_frag_idx, -1);
    }
}

@end
