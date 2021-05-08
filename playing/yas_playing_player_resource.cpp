//
//  yas_playing_rendering.cpp
//

#include "yas_playing_player_resource.h"

#include <cpp_utils/yas_fast_each.h>

#include "yas_playing_buffering_resource.h"
#include "yas_playing_reading_resource.h"

using namespace yas;
using namespace yas::playing;

player_resource::player_resource(std::shared_ptr<reading_resource_interface> const &reading,
                                 std::shared_ptr<buffering_resource_interface> const &buffering)
    : _reading(reading), _buffering(buffering) {
}

std::shared_ptr<reading_resource_interface> const &player_resource::reading() const {
    return this->_reading;
}

std::shared_ptr<buffering_resource_interface> const &player_resource::buffering() const {
    return this->_buffering;
}

void player_resource::set_playing_on_main(bool const is_playing) {
    this->_is_playing.store(is_playing);
}

bool player_resource::is_playing_on_render() const {
    return this->_is_playing.load();
}

void player_resource::seek_on_main(frame_index_t const frame) {
    std::lock_guard<std::mutex> lock(this->_seek_mutex);
    this->_seek_frame = frame;
}

std::optional<frame_index_t> player_resource::pull_seek_frame_on_render() {
    if (auto lock = std::unique_lock<std::mutex>(this->_seek_mutex, std::try_to_lock); lock.owns_lock()) {
        if (this->_seek_frame.has_value()) {
            auto frame = this->_seek_frame;
            this->_seek_frame = std::nullopt;
            return frame;
        }
    }
    return std::nullopt;
}

void player_resource::set_current_frame_on_render(frame_index_t const frame) {
    this->_current_frame.store(frame);
}

frame_index_t player_resource::current_frame() const {
    return this->_current_frame.load();
}

void player_resource::add_overwrite_request_on_main(element_address &&request) {
    std::lock_guard<std::mutex> lock(this->_overwrite_mutex);
    if (this->_is_overwritten) {
        this->_overwrite_requests.clear();
        this->_is_overwritten = false;
    }
    this->_overwrite_requests.emplace_back(std::move(request));
}

void player_resource::perform_overwrite_requests_on_render(overwrite_requests_f const &handler) {
    if (auto lock = std::unique_lock<std::mutex>(this->_overwrite_mutex, std::try_to_lock); lock.owns_lock()) {
        if (!this->_is_overwritten) {
            handler(this->_overwrite_requests);
            this->_is_overwritten = true;
        }
    }
}

void player_resource::reset_overwrite_requests_on_render() {
    if (auto lock = std::unique_lock<std::mutex>(this->_overwrite_mutex, std::try_to_lock); lock.owns_lock()) {
        this->_is_overwritten = true;
    }
}

player_resource_ptr player_resource::make_shared(std::shared_ptr<reading_resource_interface> const &reading,
                                                 std::shared_ptr<buffering_resource_interface> const &buffering) {
    return player_resource_ptr{new player_resource{reading, buffering}};
}
