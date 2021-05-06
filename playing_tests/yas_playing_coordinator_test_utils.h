//
//  yas_playing_coordinator_test_utils.h
//

#pragma once

#include <observing/yas_observing_umbrella.h>
#include <playing/yas_playing_umbrella.h>

namespace yas::playing::coordinator_test {
std::string const identifier = "0";

struct worker : workable {
    std::function<void(uint32_t, task_f &&)> add_task_handler;
    std::function<void(void)> start_handler;
    std::function<void(void)> stop_handler;

    void add_task(uint32_t const priority, task_f &&task) override {
        this->add_task_handler(priority, std::move(task));
    }

    void start() override {
        this->start_handler();
    }

    void stop() override {
        this->stop_handler();
    }
};

struct renderer : coordinator_renderable {
    std::function<void(rendering_f &&)> set_rendering_handler_handler;
    std::function<void(bool)> set_is_rendering_handler;
    std::function<playing::configuration const &(void)> configuration_handler;
    std::function<observing::syncable(configuration_observing_handler_f &&)> observe_configuration_handler;

    void set_rendering_handler(rendering_f &&handler) override {
        this->set_rendering_handler_handler(std::move(handler));
    }

    void set_is_rendering(bool const is_rendering) override {
        this->set_is_rendering_handler(is_rendering);
    }

    playing::configuration const &configuration() const override {
        return this->configuration_handler();
    }

    observing::syncable observe_configuration(configuration_observing_handler_f &&handler) override {
        return this->observe_configuration_handler(std::move(handler));
    }
};

struct player : playable {
    std::function<void(std::string)> set_identifier_handler;
    std::function<void(playing::channel_mapping)> set_ch_mapping_handler;
    std::function<void(bool)> set_playing_handler;
    std::function<void(frame_index_t)> seek_handler;
    std::function<void(std::optional<channel_index_t>, fragment_range)> overwrite_handler;
    std::function<std::string const &(void)> identifier_handler;
    std::function<playing::channel_mapping(void)> ch_mapping_handler;
    std::function<bool(void)> is_playing_handler;
    std::function<frame_index_t(void)> current_frame_handler;
    std::function<observing::syncable(std::function<void(bool const &)> &&)> observe_is_playing_handler;

    void set_identifier(std::string const &identifier) override {
        this->set_identifier_handler(identifier);
    }

    void set_channel_mapping(playing::channel_mapping const &ch_mapping) override {
        this->set_ch_mapping_handler(ch_mapping);
    }

    void set_playing(bool const is_playing) override {
        this->set_playing_handler(is_playing);
    }

    void seek(frame_index_t const frame) override {
        this->seek_handler(frame);
    }

    void overwrite(std::optional<channel_index_t> const file_ch_idx, fragment_range const frag_range) override {
        this->overwrite_handler(file_ch_idx, frag_range);
    }

    std::string const &identifier() const override {
        return this->identifier_handler();
    }

    playing::channel_mapping channel_mapping() const override {
        return this->ch_mapping_handler();
    }

    bool is_playing() const override {
        return this->is_playing_handler();
    }

    frame_index_t current_frame() const override {
        return this->current_frame_handler();
    }

    observing::syncable observe_is_playing(std::function<void(bool const &)> &&handler) override {
        return this->observe_is_playing_handler(std::move(handler));
    }
};

struct exporter : exportable {
    std::function<void(timeline_container_ptr)> set_timeline_container_handler;
    std::function<observing::endable(event_observing_handler_f &&)> observe_event_handler;

    void set_timeline_container(timeline_container_ptr const &container) override {
        this->set_timeline_container_handler(container);
    }

    observing::endable observe_event(exportable::event_observing_handler_f &&handler) override {
        return this->observe_event_handler(std::move(handler));
    }
};

struct cpp {
    std::shared_ptr<coordinator_test::worker> worker = nullptr;
    std::shared_ptr<coordinator_test::renderer> renderer = nullptr;
    std::shared_ptr<coordinator_test::player> player = nullptr;
    std::shared_ptr<coordinator_test::exporter> exporter = nullptr;

    observing::notifier_ptr<exporter_event> exporter_event_notifier = nullptr;
    observing::value::holder_ptr<configuration> configulation_holder = nullptr;
    coordinator_ptr coordinator = nullptr;

    coordinator_ptr setup_coordinator() {
        this->worker = std::make_shared<coordinator_test::worker>();
        this->renderer = std::make_shared<coordinator_test::renderer>();
        this->player = std::make_shared<coordinator_test::player>();
        this->exporter = std::make_shared<coordinator_test::exporter>();

        this->exporter_event_notifier = observing::notifier<exporter_event>::make_shared();
        this->exporter->observe_event_handler =
            [notifier = this->exporter_event_notifier](exportable::event_observing_handler_f &&handler) {
                return notifier->observe(std::move(handler));
            };

        this->configulation_holder = observing::value::holder<configuration>::make_shared(configuration{});
        this->renderer->observe_configuration_handler =
            [holder = this->configulation_holder](coordinator_renderable::configuration_observing_handler_f &&handler) {
                return holder->observe(std::move(handler));
            };

        this->worker->start_handler = [] {};

        this->coordinator = coordinator::make_shared(this->worker, this->renderer, this->player, this->exporter);

        return this->coordinator;
    }

    void reset() {
        this->worker = nullptr;
        this->renderer = nullptr;
        this->player = nullptr;
        this->exporter = nullptr;
        this->coordinator = nullptr;
    }
};
}  // namespace yas::playing::coordinator_test
