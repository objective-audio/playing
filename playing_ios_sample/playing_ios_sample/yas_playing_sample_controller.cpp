//
//  yas_playing_sample_controller.cpp
//

#include "yas_playing_sample_controller.hpp"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::sample {
struct controller_factory : controller {
    controller_factory() : controller() {
    }
};
}  // namespace yas::playing::sample

sample::controller::controller() {
}

std::shared_ptr<sample::controller> sample::make_controller() {
    return std::make_shared<sample::controller_factory>();
}
