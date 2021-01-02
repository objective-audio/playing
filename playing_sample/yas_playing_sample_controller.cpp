//
//  yas_playing_sample_controller.cpp
//

#include "yas_playing_sample_controller.hpp"

#include <iostream>

using namespace yas;
using namespace yas::playing;

namespace yas::playing::sample {
static uint32_t const length = 3;

enum class track : int64_t {
    second,
    floor,
    minus,
    pi,
    multiply,
    sine,
    level,
    gain,
    routing,
};
}  // namespace yas::playing::sample

namespace yas {
proc::track_index_t to_track_index(sample::track const track) {
    return static_cast<int64_t>(track);
}
}  // namespace yas

sample::controller::controller(audio::io_device_ptr const &device) : device(device) {
    std::cout << "root_path:" << this->root_path << std::endl;

    this->coordinator->configuration_chain()
        .to([](configuration const &config) { return config.sample_rate; })
        .send_to(this->_sample_rate)
        .sync()
        ->add_to(this->_pool);

    this->_sample_rate->chain()
        .perform([this](proc::sample_rate_t const &) { this->_update_timeline(); })
        .sync()
        ->add_to(this->_pool);

    this->frequency->chain().perform([this](float const &) { this->_update_sine_track(); }).end()->add_to(this->_pool);
}

void sample::controller::_update_timeline() {
    auto const timeline = this->make_timeline();
    this->_timeline = timeline;
    this->coordinator->set_timeline(timeline);
    this->_update_sine_track();
}

void sample::controller::_update_sine_track() {
    auto const sample_rate = this->_sample_rate->raw();

    if (sample_rate <= 0) {
        return;
    }

    proc::time::range const process_range{0, sample_rate * length};

    if (auto const &timeline = this->_timeline) {
        timeline->erase_track(to_track_index(sample::track::sine));

        if (auto sine_track = proc::track::make_shared(); true) {
            timeline->insert_track(to_track_index(sample::track::sine), sine_track);
            auto sine_module = proc::make_signal_module<float>(proc::math1::kind::sin);
            sine_module->connect_input(proc::to_connector_index(proc::math1::input::parameter), 0);
            sine_module->connect_output(proc::to_connector_index(proc::math1::output::result), 0);
            sine_track->push_back_module(std::move(sine_module), process_range);
        }
    }
}

proc::timeline_ptr sample::controller::make_timeline() {
    auto const sample_rate = this->_sample_rate->raw();
    auto const timeline = proc::timeline::make_shared();

    if (sample_rate <= 0) {
        return timeline;
    }

    proc::time::range const process_range{0, sample_rate * length};

    if (auto second_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::second), second_track);
        auto second_module = proc::make_signal_module<float>(proc::generator::kind::second, 0);
        second_module->connect_output(proc::to_connector_index(proc::generator::output::value), 0);
        second_track->push_back_module(std::move(second_module), process_range);
    }

    if (auto floor_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::floor), floor_track);
        auto floor_module = proc::make_signal_module<float>(proc::math1::kind::floor);
        floor_module->connect_input(proc::to_connector_index(proc::math1::input::parameter), 0);
        floor_module->connect_output(proc::to_connector_index(proc::math1::output::result), 1);
        floor_track->push_back_module(std::move(floor_module), process_range);
    }

    if (auto minus_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::minus), minus_track);
        auto minus_module = proc::make_signal_module<float>(proc::math2::kind::minus);
        minus_module->connect_input(proc::to_connector_index(proc::math2::input::left), 0);
        minus_module->connect_input(proc::to_connector_index(proc::math2::input::right), 1);
        minus_module->connect_output(proc::to_connector_index(proc::math2::output::result), 0);
        minus_track->push_back_module(std::move(minus_module), process_range);
    }

    if (auto pi_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::pi), pi_track);
        auto pi_module = proc::make_signal_module<float>(2.0f * M_PI * this->frequency->raw());
        pi_module->connect_output(proc::to_connector_index(proc::constant::output::value), 1);
        pi_track->push_back_module(std::move(pi_module), process_range);
    }

    if (auto multiply_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::multiply), multiply_track);
        auto multiply_module = proc::make_signal_module<float>(proc::math2::kind::multiply);
        multiply_module->connect_input(proc::to_connector_index(proc::math2::input::left), 0);
        multiply_module->connect_input(proc::to_connector_index(proc::math2::input::right), 1);
        multiply_module->connect_output(proc::to_connector_index(proc::math2::output::result), 0);
        multiply_track->push_back_module(std::move(multiply_module), process_range);
    }

    if (auto level_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::level), level_track);
        auto level_module = proc::make_signal_module<float>(0.1f);
        level_module->connect_output(proc::to_connector_index(proc::constant::output::value), 1);
        level_track->push_back_module(std::move(level_module), process_range);
    }

    if (auto gain_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::gain), gain_track);
        auto gain_module = proc::make_signal_module<float>(proc::math2::kind::multiply);
        gain_module->connect_input(proc::to_connector_index(proc::math2::input::left), 0);
        gain_module->connect_input(proc::to_connector_index(proc::math2::input::right), 1);
        gain_module->connect_output(proc::to_connector_index(proc::math2::output::result), 0);
        gain_track->push_back_module(std::move(gain_module), process_range);
    }

    if (auto routing_track = proc::track::make_shared(); true) {
        timeline->insert_track(to_track_index(sample::track::routing), routing_track);
        auto routing_module = proc::make_signal_module<float>(proc::routing::kind::copy);
        routing_module->connect_input(proc::to_connector_index(proc::routing::input::value), 0);
        routing_module->connect_output(proc::to_connector_index(proc::routing::output::value), 1);
        routing_track->push_back_module(routing_module, process_range);
    }

    return timeline;
}

std::shared_ptr<sample::controller> sample::controller::make_shared(audio::io_device_ptr const &device) {
    return std::shared_ptr<sample::controller>(new sample::controller{device});
}
