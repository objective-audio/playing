//
//  yas_playing_buffering_element_types.cpp
//

#include "yas_playing_buffering_element_types.h"

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
