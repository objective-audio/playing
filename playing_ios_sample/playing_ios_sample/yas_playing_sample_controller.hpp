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
    audio::io_device_ptr const device = audio::ios_device::make_renewable_device(audio::ios_session::shared());
    audio_coordinator_ptr const coordinator = audio_coordinator::make_shared(this->root_path, this->device);
    playing::timeline_exporter_ptr const timeline_exporter =
        playing::timeline_exporter::make_shared(this->root_path, std::make_shared<task_queue>(2),
                                                {.timeline = 0, .fragment = 1}, this->coordinator->sample_rate());
    chaining::observer_pool pool;

    static std::shared_ptr<controller> make_shared();

   private:
    controller();

    proc::timeline_ptr make_sine_timeline(proc::sample_rate_t const);
};
}  // namespace yas::playing::sample
