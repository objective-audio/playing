//
//  yas_playing_sample_controller.cpp
//

#include "yas_playing_sample_controller.hpp"

#include <iostream>

using namespace yas;
using namespace yas::playing;

sample::controller::controller() {
    proc::sample_rate_t const sample_rate = this->coordinator->sample_rate();
    auto timeline = this->make_sine_timeline(sample_rate);
    auto container = timeline_container::make_shared("0", sample_rate, timeline);
    this->timeline_exporter->set_timeline_container(container);
}

proc::timeline_ptr sample::controller::make_sine_timeline(proc::sample_rate_t const sample_rate) {
    uint32_t const length = 3;
    proc::time::range const process_range{0, sample_rate * length};

    auto const timeline = proc::timeline::make_shared();
    proc::track_index_t trk_idx = 0;

    if (auto second_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, second_track);
        auto second_module = proc::make_signal_module<float>(proc::generator::kind::second, 0);
        second_module->connect_output(proc::to_connector_index(proc::generator::output::value), 0);
        second_track->push_back_module(std::move(second_module), process_range);
    }

    if (auto floor_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, floor_track);
        auto floor_module = proc::make_signal_module<float>(proc::math1::kind::floor);
        floor_module->connect_input(proc::to_connector_index(proc::math1::input::parameter), 0);
        floor_module->connect_output(proc::to_connector_index(proc::math1::output::result), 1);
        floor_track->push_back_module(std::move(floor_module), process_range);
    }

    if (auto minus_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, minus_track);
        auto minus_module = proc::make_signal_module<float>(proc::math2::kind::minus);
        minus_module->connect_input(proc::to_connector_index(proc::math2::input::left), 0);
        minus_module->connect_input(proc::to_connector_index(proc::math2::input::right), 1);
        minus_module->connect_output(proc::to_connector_index(proc::math2::output::result), 0);
        minus_track->push_back_module(std::move(minus_module), process_range);
    }

    if (auto pi_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, pi_track);
        auto pi_module = proc::make_signal_module<float>(2.0f * M_PI * 1000.0f);
        pi_module->connect_output(proc::to_connector_index(proc::constant::output::value), 1);
        pi_track->push_back_module(std::move(pi_module), process_range);
    }

    if (auto multiply_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, multiply_track);
        auto multiply_module = proc::make_signal_module<float>(proc::math2::kind::multiply);
        multiply_module->connect_input(proc::to_connector_index(proc::math2::input::left), 0);
        multiply_module->connect_input(proc::to_connector_index(proc::math2::input::right), 1);
        multiply_module->connect_output(proc::to_connector_index(proc::math2::output::result), 0);
        multiply_track->push_back_module(std::move(multiply_module), process_range);
    }

    if (auto sine_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, sine_track);
        auto sine_module = proc::make_signal_module<float>(proc::math1::kind::sin);
        sine_module->connect_input(proc::to_connector_index(proc::math1::input::parameter), 0);
        sine_module->connect_output(proc::to_connector_index(proc::math1::output::result), 0);
        sine_track->push_back_module(std::move(sine_module), process_range);
    }

    if (auto level_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, level_track);
        auto level_module = proc::make_signal_module<float>(0.1f);
        level_module->connect_output(proc::to_connector_index(proc::constant::output::value), 1);
        level_track->push_back_module(std::move(level_module), process_range);
    }

    if (auto gain_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, gain_track);
        auto gain_module = proc::make_signal_module<float>(proc::math2::kind::multiply);
        gain_module->connect_input(proc::to_connector_index(proc::math2::input::left), 0);
        gain_module->connect_input(proc::to_connector_index(proc::math2::input::right), 1);
        gain_module->connect_output(proc::to_connector_index(proc::math2::output::result), 0);
        gain_track->push_back_module(std::move(gain_module), process_range);
    }

    if (auto routing_track = proc::track::make_shared(); true) {
        timeline->insert_track(trk_idx++, routing_track);
        auto routing_module = proc::make_signal_module<float>(proc::routing::kind::copy);
        routing_module->connect_input(proc::to_connector_index(proc::routing::input::value), 0);
        routing_module->connect_output(proc::to_connector_index(proc::routing::output::value), 1);
        routing_track->push_back_module(routing_module, process_range);
    }

    return timeline;
}

std::shared_ptr<sample::controller> sample::controller::make_shared() {
    auto result = audio::ios_session::shared()->activate();
    return std::shared_ptr<sample::controller>(new sample::controller{});
}
