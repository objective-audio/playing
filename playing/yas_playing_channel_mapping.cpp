//
//  yas_playing_channel_mapping.cpp
//

#include "yas_playing_channel_mapping.h"

using namespace yas;
using namespace yas::playing;

channel_mapping::channel_mapping(std::vector<channel_index_t> &&indices) : indices(indices) {
}

channel_mapping_ptr channel_mapping::make_shared() {
    return make_shared({});
}

channel_mapping_ptr channel_mapping::make_shared(std::vector<channel_index_t> &&indices) {
    return std::shared_ptr<channel_mapping>(new channel_mapping{std::move(indices)});
}
