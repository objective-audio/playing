//
//  yas_playing_loading_state.h
//

#pragma once

#include "yas_playing_types.h"

namespace yas::playing {
enum class loading_kind {
    unloaded,
    loaded,
};

struct loading_state {
    using ptr = std::shared_ptr<loading_state>;

    std::optional<fragment_index_t> const frag_idx;
    loading_kind const kind;

    loading_state();
    loading_state(std::optional<fragment_index_t> const, loading_kind const);

    bool operator==(loading_state const &rhs) const;
};
}  // namespace yas::playing

namespace yas {
std::string to_string(playing::loading_kind const &);
}

std::ostream &operator<<(std::ostream &, yas::playing::loading_kind const &);
