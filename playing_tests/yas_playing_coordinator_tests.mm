//
//  yas_playing_coordinator_tests.mm
//

#import <XCTest/XCTest.h>
#import <chaining/chaining.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

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
    std::function<sample_rate_t(void)> sample_rate_handler;
    std::function<audio::pcm_format(void)> pcm_format_handler;
    std::function<std::size_t(void)> channel_count_handler;
    std::function<chaining::chain_sync_t<configuration>(void)> configuration_chain_handler;

    void set_rendering_handler(rendering_f &&handler) override {
        this->set_rendering_handler_handler(std::move(handler));
    }

    void set_is_rendering(bool const is_rendering) override {
        this->set_is_rendering_handler(is_rendering);
    }

    sample_rate_t sample_rate() const override {
        return this->sample_rate_handler();
    }

    audio::pcm_format pcm_format() const override {
        return this->pcm_format_handler();
    }

    std::size_t channel_count() const override {
        return this->channel_count_handler();
    }

    chaining::chain_sync_t<configuration> configuration_chain() const override {
        return this->configuration_chain_handler();
    }

    chaining::chain_sync_t<configuration> configuration_chain2() const {
        return this->configuration_chain_handler();
    }
};

struct player : playable {
    std::function<void(channel_mapping_ptr)> set_ch_mapping_handler;
    std::function<void(bool)> set_playing_handler;
    std::function<void(frame_index_t)> seek_handler;
    std::function<void(std::optional<channel_index_t>, fragment_index_t)> overwrite_handler;
    std::function<channel_mapping_ptr const &(void)> ch_mapping_handler;
    std::function<bool(void)> is_playing_handler;
    std::function<frame_index_t(void)> current_frame_handler;
    std::function<chaining::chain_sync_t<bool>(void)> is_playing_chain_handler;

    void set_channel_mapping(channel_mapping_ptr const &ch_mapping) override {
        this->set_ch_mapping_handler(ch_mapping);
    }

    void set_playing(bool const is_playing) override {
        this->set_playing_handler(is_playing);
    }

    void seek(frame_index_t const frame) override {
        this->seek_handler(frame);
    }

    void overwrite(std::optional<channel_index_t> const file_ch_idx, fragment_index_t const frag_idx) override {
        this->overwrite_handler(file_ch_idx, frag_idx);
    }

    channel_mapping_ptr const &channel_mapping() const override {
        return this->ch_mapping_handler();
    }

    bool is_playing() const override {
        return this->is_playing_handler();
    }

    frame_index_t current_frame() const override {
        return this->current_frame_handler();
    }

    chaining::chain_sync_t<bool> is_playing_chain() const override {
        return this->is_playing_chain_handler();
    }
};

struct exporter : exportable {
    std::function<void(timeline_container_ptr)> set_timeline_container_handler;
    std::function<chaining::chain_unsync_t<exporter_event>(void)> event_chain_handler;

    void set_timeline_container(timeline_container_ptr const &container) override {
        this->set_timeline_container_handler(container);
    }

    chaining::chain_unsync_t<exporter_event> event_chain() const override {
        return this->event_chain_handler();
    }
};

struct coordinator_cpp {
    std::shared_ptr<coordinator_test::worker> worker = nullptr;
    std::shared_ptr<coordinator_test::renderer> renderer = nullptr;
    std::shared_ptr<coordinator_test::player> player = nullptr;
    std::shared_ptr<coordinator_test::exporter> exporter = nullptr;

    chaining::notifier_ptr<exporter_event> exporter_event_notifier = nullptr;
    chaining::value::holder_ptr<configuration> configulation_holder = nullptr;
    coordinator_ptr coordinator = nullptr;

    coordinator_ptr setup_coordinator() {
        this->worker = std::make_shared<coordinator_test::worker>();
        this->renderer = std::make_shared<coordinator_test::renderer>();
        this->player = std::make_shared<coordinator_test::player>();
        this->exporter = std::make_shared<coordinator_test::exporter>();

        this->exporter_event_notifier = chaining::notifier<exporter_event>::make_shared();
        this->exporter->event_chain_handler = [notifier = this->exporter_event_notifier] { return notifier->chain(); };

        this->configulation_holder = chaining::value::holder<configuration>::make_shared(configuration{});
        this->renderer->configuration_chain_handler = [holder = this->configulation_holder] { return holder->chain(); };

        this->worker->start_handler = [] {};

        this->coordinator = coordinator::make_shared(coordinator_test::identifier, this->worker, this->renderer,
                                                     this->player, this->exporter);

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
}

@interface yas_playing_coordinator_tests : XCTestCase

@end

@implementation yas_playing_coordinator_tests {
    coordinator_test::coordinator_cpp _cpp;
}

- (void)tearDown {
    self->_cpp.reset();

    [super tearDown];
}

- (void)test_constructor {
    auto const worker = std::make_shared<coordinator_test::worker>();
    auto const renderer = std::make_shared<coordinator_test::renderer>();
    auto const player = std::make_shared<coordinator_test::player>();
    auto const exporter = std::make_shared<coordinator_test::exporter>();

    bool exporter_event_called = false;
    bool configuration_chain_called = false;
    bool start_called = false;

    auto const exporter_event_notifier = chaining::notifier<exporter_event>::make_shared();
    exporter->event_chain_handler = [notifier = exporter_event_notifier, &exporter_event_called] {
        exporter_event_called = true;
        return notifier->chain();
    };

    auto const configulation_holder = chaining::value::holder<configuration>::make_shared(configuration{});
    renderer->configuration_chain_handler = [holder = configulation_holder, &configuration_chain_called] {
        configuration_chain_called = true;
        return holder->chain();
    };

    worker->start_handler = [&start_called] { start_called = true; };

    auto const coordinator = coordinator::make_shared(coordinator_test::identifier, worker, renderer, player, exporter);

    XCTAssertTrue(exporter_event_called);
    XCTAssertTrue(configuration_chain_called);
    XCTAssertTrue(start_called);
}

- (void)test_set_timeline {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<timeline_container_ptr> called;

    self->_cpp.exporter->set_timeline_container_handler = [&called](auto const &container) {
        called.emplace_back(container);
    };
    self->_cpp.renderer->sample_rate_handler = [] { return 44100; };

    auto chain = self->_cpp.renderer->configuration_chain();

    XCTAssertFalse(coordinator->timeline().has_value());

    auto const timeline = proc::timeline::make_shared();

    coordinator->set_timeline(timeline);

    XCTAssertEqual(coordinator->timeline(), timeline);
    XCTAssertEqual(called.size(), 1);

    auto const &container = called.at(0);
    XCTAssertEqual(container->identifier(), coordinator_test::identifier);
    XCTAssertEqual(container->timeline(), timeline);
    XCTAssertEqual(container->sample_rate(), 44100);
}

- (void)test_set_channel_mapping {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<channel_mapping_ptr> called;

    self->_cpp.player->set_ch_mapping_handler = [&called](channel_mapping_ptr const &ch_mapping) {
        called.emplace_back(ch_mapping);
    };

    auto const ch_mapping = channel_mapping::make_shared({3, 2, 1});

    coordinator->set_channel_mapping(ch_mapping);

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0), ch_mapping);
}

- (void)test_set_playing {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<bool> called;

    self->_cpp.player->set_playing_handler = [&called](bool is_playing) { called.emplace_back(is_playing); };

    coordinator->set_playing(true);

    XCTAssertEqual(called.size(), 1);
    XCTAssertTrue(called.at(0));

    coordinator->set_playing(false);

    XCTAssertEqual(called.size(), 2);
    XCTAssertFalse(called.at(1));
}

- (void)test_seek {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<frame_index_t> called;

    self->_cpp.player->seek_handler = [&called](frame_index_t frame) { called.emplace_back(frame); };

    coordinator->seek(123);

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0), 123);
}

- (void)test_overwrite {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<std::pair<std::optional<channel_index_t>, fragment_index_t>> called;

    self->_cpp.player->overwrite_handler = [&called](std::optional<channel_index_t> file_ch_idx,
                                                     fragment_index_t frag_idx) {
        called.emplace_back(file_ch_idx, frag_idx);
    };

    self->_cpp.renderer->sample_rate_handler = [] { return 4; };

    coordinator->overwrite(proc::time::range{0, 4});

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second, 0);

    called.clear();

    coordinator->overwrite(proc::time::range{0, 5});

    XCTAssertEqual(called.size(), 2);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second, 0);
    XCTAssertEqual(called.at(1).first, std::nullopt);
    XCTAssertEqual(called.at(1).second, 1);

    called.clear();

    coordinator->overwrite(proc::time::range{-1, 6});

    XCTAssertEqual(called.size(), 3);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second, -1);
    XCTAssertEqual(called.at(1).first, std::nullopt);
    XCTAssertEqual(called.at(1).second, 0);
    XCTAssertEqual(called.at(2).first, std::nullopt);
    XCTAssertEqual(called.at(2).second, 1);
}

- (void)test_identifier {
    auto const coordinator = self->_cpp.setup_coordinator();

    XCTAssertEqual(coordinator->identifier(), coordinator_test::identifier);
}

- (void)test_channel_mapping {
    auto const coordinator = self->_cpp.setup_coordinator();

    auto const ch_mapping = channel_mapping::make_shared({2, 3});

    self->_cpp.player->ch_mapping_handler = [&ch_mapping] { return ch_mapping; };

    XCTAssertEqual(coordinator->channel_mapping(), ch_mapping);
}

- (void)test_is_playing {
    auto const coordinator = self->_cpp.setup_coordinator();

    bool is_playing = false;

    self->_cpp.player->is_playing_handler = [&is_playing] { return is_playing; };

    XCTAssertFalse(coordinator->is_playing());

    is_playing = true;

    XCTAssertTrue(coordinator->is_playing());
}

- (void)test_current_frame {
    auto const coordinator = self->_cpp.setup_coordinator();

    frame_index_t frame = 1;

    self->_cpp.player->current_frame_handler = [&frame] { return frame; };

    XCTAssertEqual(coordinator->current_frame(), 1);

    frame = 2;

    XCTAssertEqual(coordinator->current_frame(), 2);
}

- (void)test_sample_rate {
    auto const coordinator = self->_cpp.setup_coordinator();

    sample_rate_t sample_rate = 1000;

    self->_cpp.renderer->sample_rate_handler = [&sample_rate] { return sample_rate; };

    XCTAssertEqual(coordinator->sample_rate(), 1000);

    sample_rate = 2000;

    XCTAssertEqual(coordinator->sample_rate(), 2000);
}

- (void)test_pcm_format {
    auto const coordinator = self->_cpp.setup_coordinator();

    auto pcm_format = audio::pcm_format::int16;

    self->_cpp.renderer->pcm_format_handler = [&pcm_format] { return pcm_format; };

    XCTAssertEqual(coordinator->pcm_format(), audio::pcm_format::int16);

    pcm_format = audio::pcm_format::float32;

    XCTAssertEqual(coordinator->pcm_format(), audio::pcm_format::float32);
}

- (void)test_channel_count {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::size_t ch_count = 1;

    self->_cpp.renderer->channel_count_handler = [&ch_count] { return ch_count; };

    XCTAssertEqual(coordinator->channel_count(), 1);

    ch_count = 2;

    XCTAssertEqual(coordinator->channel_count(), 2);
}

- (void)test_configuration_chain {
    auto const coordinator = self->_cpp.setup_coordinator();
    chaining::observer_pool pool;

    std::vector<timeline_container_ptr> called_containers;

    self->_cpp.exporter->set_timeline_container_handler = [&called_containers](timeline_container_ptr container) {
        called_containers.emplace_back(container);
    };

    // configuration_chainとは別で返す（実際は同じ値になる）
    self->_cpp.renderer->sample_rate_handler = [] { return 5; };

    std::vector<configuration> called_configrations;

    coordinator->configuration_chain()
        .perform([&called_configrations](auto const &config) { called_configrations.emplace_back(config); })
        .sync()
        ->add_to(pool);

    XCTAssertEqual(called_configrations.size(), 1);
    XCTAssertEqual(called_configrations.at(0), (configuration{}));
    XCTAssertEqual(called_containers.size(), 0);

    self->_cpp.configulation_holder->set_value(
        {.sample_rate = 4, .pcm_format = audio::pcm_format::float32, .channel_count = 1});

    XCTAssertEqual(called_configrations.size(), 2);
    XCTAssertEqual(called_configrations.at(1),
                   (configuration{.sample_rate = 4, .pcm_format = audio::pcm_format::float32, .channel_count = 1}));
    XCTAssertEqual(called_containers.size(), 1);
    XCTAssertEqual(called_containers.at(0)->sample_rate(), 5);
}

- (void)test_is_playing_chain {
    auto const coordinator = self->_cpp.setup_coordinator();
    chaining::observer_pool pool;

    auto const is_playing = chaining::value::holder<bool>::make_shared(false);

    self->_cpp.player->is_playing_chain_handler = [&is_playing] { return is_playing->chain(); };

    std::vector<bool> called;

    coordinator->is_playing_chain()
        .perform([&called](bool const &is_playing) { called.emplace_back(is_playing); })
        .sync()
        ->add_to(pool);

    XCTAssertEqual(called.size(), 1);
    XCTAssertFalse(called.at(0));

    is_playing->set_value(true);

    XCTAssertEqual(called.size(), 2);
    XCTAssertTrue(called.at(1));
}

- (void)test_export {
    auto const coordinator = self->_cpp.setup_coordinator();

    self->_cpp.renderer->sample_rate_handler = [] { return 4; };

    std::vector<std::pair<std::optional<channel_index_t>, fragment_index_t>> called;

    self->_cpp.player->overwrite_handler = [&called](std::optional<channel_index_t> ch_idx, fragment_index_t frag_idx) {
        called.emplace_back(std::make_pair(ch_idx, frag_idx));
    };

    self->_cpp.exporter_event_notifier->notify(
        exporter_event{.result = exporter_result_t{exporter_method::export_ended}, .range = proc::time::range{0, 1}});

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second, 0);

    self->_cpp.exporter_event_notifier->notify(
        exporter_event{.result = exporter_result_t{exporter_method::export_ended}, .range = proc::time::range{-1, 6}});

    XCTAssertEqual(called.size(), 4);
    XCTAssertEqual(called.at(1).first, std::nullopt);
    XCTAssertEqual(called.at(1).second, -1);
    XCTAssertEqual(called.at(2).first, std::nullopt);
    XCTAssertEqual(called.at(2).second, 0);
    XCTAssertEqual(called.at(3).first, std::nullopt);
    XCTAssertEqual(called.at(3).second, 1);
}

@end
