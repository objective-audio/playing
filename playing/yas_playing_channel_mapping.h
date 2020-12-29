//
//  yas_playing_channel_mapping.h
//

#pragma once

#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>

#include <vector>

namespace yas::playing {
struct channel_mapping {
    std::vector<channel_index_t> const indices;

    static channel_mapping_ptr make_shared();
    static channel_mapping_ptr make_shared(std::vector<channel_index_t> &&);

   private:
    channel_mapping(std::vector<channel_index_t> &&);
};
};  // namespace yas::playing
