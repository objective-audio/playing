//
//  yas_playing_audio_utils.cpp
//

#include "yas_playing_audio_utils.h"

#include <cpp_utils/yas_fast_each.h>

#include <thread>

#include "yas_playing_audio_buffering_channel.h"
#include "yas_playing_audio_buffering_element.h"

using namespace yas;
using namespace yas::playing;

std::shared_ptr<playing::audio_buffering_channel> audio_utils::make_channel(std::size_t const element_count,
                                                                            audio::format const &format,
                                                                            sample_rate_t const frag_length) {
    std::vector<audio_buffering_element_protocol_ptr> elements;
    elements.reserve(element_count);

    auto element_each = make_fast_each(element_count);
    while (yas_each_next(element_each)) {
        elements.emplace_back(audio_buffering_element::make_shared(format, frag_length));
        std::this_thread::yield();
    }

    return playing::audio_buffering_channel::make_shared(std::move(elements));
}
