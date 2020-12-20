//
//  yas_playing_audio_buffering_element_protocol.cpp
//

#include "yas_playing_audio_buffering_element_protocol.h"

std::string yas::to_string(playing::audio_buffering_element_state const &state) {
    switch (state) {
        case playing::audio_buffering_element_state::initial:
            return "initial";
        case playing::audio_buffering_element_state::writable:
            return "writable";
        case playing::audio_buffering_element_state::readable:
            return "readable";
    }
}
