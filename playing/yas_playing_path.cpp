//
//  yas_playing_url.cpp
//

#include "yas_playing_path.h"

#include <cpp_utils/yas_boolean.h>

#include "yas_playing_math.h"
#include "yas_playing_signal_file_info.h"

using namespace yas;
using namespace yas::playing;
using namespace yas::playing::path;

#pragma mark - path::timeline

std::string timeline::string() const {
    return file_path{this->root_path}.appending(timeline_name(this->identifier, this->sample_rate)).string();
}

bool timeline::operator==(timeline const &rhs) const {
    return this->root_path == rhs.root_path && this->identifier == rhs.identifier &&
           this->sample_rate == rhs.sample_rate;
}

bool timeline::operator!=(timeline const &rhs) const {
    return !(*this == rhs);
}

#pragma mark - path::channel

std::string channel::string() const {
    return file_path{this->timeline_path.string()}.appending(channel_name(this->channel_index)).string();
}

bool channel::operator==(channel const &rhs) const {
    return this->timeline_path == rhs.timeline_path && this->channel_index == rhs.channel_index;
}

bool channel::operator!=(channel const &rhs) const {
    return !(*this == rhs);
}

#pragma mark - path::fragment

std::string fragment::string() const {
    return file_path{this->channel_path.string()}.appending(fragment_name(this->fragment_index)).string();
}

bool fragment::operator==(fragment const &rhs) const {
    return this->channel_path == rhs.channel_path && this->fragment_index == rhs.fragment_index;
}

bool fragment::operator!=(fragment const &rhs) const {
    return !(*this == rhs);
}

#pragma mark - path::signal_event

std::string signal_event::string() const {
    return file_path{this->fragment_path.string()}
        .appending(to_signal_file_name(this->range, this->sample_type))
        .string();
}

bool signal_event::operator==(signal_event const &rhs) const {
    return this->fragment_path == rhs.fragment_path && this->range == rhs.range && this->sample_type == rhs.sample_type;
}

bool signal_event::operator!=(signal_event const &rhs) const {
    return !(*this == rhs);
}

#pragma mark - path::number_events

std::string number_events::string() const {
    return file_path{this->fragment_path.string()}.appending("numbers").string();
}

bool number_events::operator==(number_events const &rhs) const {
    return this->fragment_path == rhs.fragment_path;
}

bool number_events::operator!=(number_events const &rhs) const {
    return !(*this == rhs);
}

#pragma mark - name

std::string path::timeline_name(std::string const &identifier, sample_rate_t const sr) {
    return identifier + "_" + std::to_string(sr);
}

std::string path::channel_name(channel_index_t const ch_idx) {
    return std::to_string(ch_idx);
}

std::string path::fragment_name(fragment_index_t const frag_idx) {
    return std::to_string(frag_idx);
}
