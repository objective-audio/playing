//
//  yas_playing_url.h
//

#pragma once

#include <cpp_utils/yas_file_path.h>
#include <cpp_utils/yas_url.h>
#include <playing/yas_playing_types.h>
#include <processing/yas_processing_time.h>

namespace yas::playing::path {
struct [[nodiscard]] timeline final {
    std::string root_path;
    std::string identifier;
    sample_rate_t sample_rate;

    [[nodiscard]] std::string string() const;

    bool operator==(timeline const &rhs) const;
    bool operator!=(timeline const &rhs) const;
};

struct [[nodiscard]] channel final {
    timeline timeline_path;
    channel_index_t channel_index;

    [[nodiscard]] std::string string() const;

    bool operator==(channel const &rhs) const;
    bool operator!=(channel const &rhs) const;
};

struct [[nodiscard]] fragment final {
    channel channel_path;
    fragment_index_t fragment_index;

    [[nodiscard]] std::string string() const;

    bool operator==(fragment const &rhs) const;
    bool operator!=(fragment const &rhs) const;
};

struct [[nodiscard]] signal_event final {
    fragment fragment_path;
    proc::time::range range;
    std::type_info const &sample_type;

    [[nodiscard]] std::string string() const;

    bool operator==(signal_event const &rhs) const;
    bool operator!=(signal_event const &rhs) const;
};

struct [[nodiscard]] number_events final {
    fragment fragment_path;

    [[nodiscard]] std::string string() const;

    bool operator==(number_events const &rhs) const;
    bool operator!=(number_events const &rhs) const;
};

[[nodiscard]] std::string timeline_name(std::string const &identifier, sample_rate_t const);
[[nodiscard]] std::string channel_name(channel_index_t const ch_idx);
[[nodiscard]] std::string fragment_name(fragment_index_t const frag_idx);
}  // namespace yas::playing::path
