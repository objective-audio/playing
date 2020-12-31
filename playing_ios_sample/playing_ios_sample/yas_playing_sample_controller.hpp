//
//  yas_playing_sample_controller.hpp
//

#pragma once

#include <audio/yas_audio_umbrella.h>
#include <cpp_utils/yas_cpp_utils_umbrella.h>
#include <playing/yas_playing_umbrella.h>
#include <processing/yas_processing_umbrella.h>

namespace yas::playing::sample {
struct controller {
    std::string const root_path =
        file_path{system_path_utils::directory_path(system_path_utils::dir::document)}.appending("sample").string();
    std::string const identifier = "0";
    audio::io_device_ptr const device = audio::ios_device::make_renewable_device(audio::ios_session::shared());
    coordinator_ptr const coordinator = coordinator::make_shared(this->root_path, this->identifier, this->device);
    chaining::observer_pool pool;

    static std::shared_ptr<controller> make_shared();

   private:
    controller();

    proc::timeline_ptr make_sine_timeline(proc::sample_rate_t const);
};
}  // namespace yas::playing::sample
