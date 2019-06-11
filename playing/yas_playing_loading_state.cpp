//
//  yas_playing_loading_state.cpp
//

#include "yas_playing_loading_state.h"
#include <string>

using namespace yas;
using namespace yas::playing;

loading_state::loading_state() : frag_idx(std::nullopt), kind(loading_kind::unloaded) {
}

loading_state::loading_state(std::optional<fragment_index_t> const frag_idx, loading_kind const kind)
    : frag_idx(frag_idx), kind(kind) {
}

bool loading_state::operator==(loading_state const &rhs) const {
    if (this->frag_idx && rhs.frag_idx) {
        return *this->frag_idx == *rhs.frag_idx && this->kind == rhs.kind;
    } else if (!this->frag_idx && !rhs.frag_idx) {
        return this->kind == rhs.kind;
    } else {
        return false;
    }
}

#pragma mark -

std::string yas::to_string(const playing::loading_kind &kind) {
    switch (kind) {
        case playing::loading_kind::loaded:
            return "loaded";
        case playing::loading_kind::unloaded:
            return "unloaded";
    }
}

std::ostream &operator<<(std::ostream &stream, yas::playing::loading_kind const &value) {
    stream << to_string(value);
    return stream;
}
