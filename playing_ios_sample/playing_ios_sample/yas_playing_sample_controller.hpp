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
        std::string root_path =
        file_path{system_path_utils::directory_path(system_path_utils::dir::document)}.appending("sample").string();
        audio_coordinator coordinator{this->root_path};
        playing::timeline_exporter timeline_exporter{this->root_path, task_queue{2}, {.timeline = 0, .fragment = 1}, this->coordinator.sample_rate()};
        chaining::observer_pool pool;
        
    protected:
        controller();
    };
    
    std::shared_ptr<controller> make_controller();
}
