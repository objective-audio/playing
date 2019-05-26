//
//  yas_playing_test_utils.h
//

#pragma once

#include <processing/processing.h>

namespace yas::playing::test_utils {
std::string root_path();
proc::timeline test_timeline(int64_t const offset, uint32_t const ch_count);
}  // namespace yas::playing::test_utils
