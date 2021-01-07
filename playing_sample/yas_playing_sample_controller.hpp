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
    audio::io_device_ptr const device;
    std::string const root_path =
        file_path{system_path_utils::directory_path(system_path_utils::dir::document)}.appending("sample").string();
    std::string const identifier = "0";
    coordinator_ptr const coordinator = coordinator::make_shared(this->root_path, this->identifier, this->device);

    chaining::value::holder_ptr<float> const frequency = chaining::value::holder<float>::make_shared(1000.0f);

    static std::shared_ptr<controller> make_shared(audio::io_device_ptr const &);

   private:
    proc::timeline_ptr _timeline = nullptr;
    chaining::value::holder_ptr<sample_rate_t> const _sample_rate =
        chaining::value::holder<sample_rate_t>::make_shared(0);

    chaining::observer_pool _pool;

    controller(audio::io_device_ptr const &);

    void _update_timeline();
    void _update_pi_track();
    proc::timeline_ptr make_timeline();
};
}  // namespace yas::playing::sample
