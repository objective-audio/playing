//
//  yas_playing_audio_utils.h
//

#pragma once

#include <audio/yas_audio_format.h>
#include <playing/yas_playing_types.h>

#include <memory>

namespace yas::playing {
class buffering_channel;
}

namespace yas::playing::audio_utils {
[[nodiscard]] std::shared_ptr<buffering_channel> make_channel(std::size_t const element_count,
                                                              audio::format const &format,
                                                              sample_rate_t const frag_length);
}  // namespace yas::playing::audio_utils
