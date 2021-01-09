//
//  yas_playing_channel_mapping.h
//

#pragma once

#include <playing/yas_playing_ptr.h>
#include <playing/yas_playing_types.h>

#include <vector>

namespace yas::playing {
struct channel_mapping final {
    std::vector<channel_index_t> const indices;

    std::optional<channel_index_t> file_index(channel_index_t const out_idx, std::size_t const ch_count) const;
    std::optional<channel_index_t> out_index(channel_index_t const file_idx, std::size_t const ch_count) const;

    static channel_mapping_ptr make_shared();
    static channel_mapping_ptr make_shared(std::vector<channel_index_t> &&);

   private:
    channel_mapping(std::vector<channel_index_t> &&);
};
};  // namespace yas::playing
