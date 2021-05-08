//
//  yas_playing_reading_resource_protocol.h
//

#pragma once

#include <playing/yas_playing_types.h>

namespace yas::playing {
enum class reading_resource_state {
    /// 起動した状態
    /// render側: creatingにする
    /// task側: 何もしない
    initial,

    /// バッファを作る状態
    /// render側: 何もしない
    /// task側: バッファを作る
    creating,

    /// bufferがある状態
    /// render側: レンダリングに使う。フォーマットが合わなければcreatingにする。
    /// task側: 何もしない
    rendering,
};
}  // namespace yas::playing
