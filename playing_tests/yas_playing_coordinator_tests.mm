//
//  yas_playing_coordinator_tests.mm
//

#import <XCTest/XCTest.h>
#import "yas_playing_coordinator_test_utils.h"

using namespace yas;
using namespace yas::playing;

@interface yas_playing_coordinator_tests : XCTestCase

@end

@implementation yas_playing_coordinator_tests {
    coordinator_test::cpp _cpp;
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

    auto const exporter_event_notifier = observing::notifier<exporter_event>::make_shared();
    exporter->observe_event_handler = [notifier = exporter_event_notifier,
                                       &exporter_event_called](exportable::event_observing_handler_f &&handler) {
        exporter_event_called = true;
        return notifier->observe(std::move(handler));
    };

    auto const configulation_holder = observing::value::holder<configuration>::make_shared(configuration{});
    renderer->observe_configuration_handler = [holder = configulation_holder, &configuration_chain_called](
                                                  coordinator_renderable::configuration_observing_handler_f &&handler) {
        configuration_chain_called = true;
        return holder->observe(std::move(handler));
    };

    worker->start_handler = [&start_called] { start_called = true; };

    auto const coordinator = coordinator::make_shared(worker, renderer, player, exporter);

    XCTAssertTrue(exporter_event_called);
    XCTAssertTrue(configuration_chain_called);
    XCTAssertTrue(start_called);
}

- (void)test_set_and_reset_timeline {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<timeline_container_ptr> called_set_container;
    std::vector<std::string> called_set_identifier;

    self->_cpp.exporter->set_timeline_container_handler = [&called_set_container](auto const &container) {
        called_set_container.emplace_back(container);
    };
    playing::configuration configuration{.sample_rate = 44100};
    self->_cpp.renderer->configuration_handler = [&configuration] { return configuration; };
    self->_cpp.player->set_identifier_handler = [&called_set_identifier](std::string const &identifier) {
        called_set_identifier.emplace_back(identifier);
    };

    XCTAssertFalse(coordinator->timeline().has_value());
    XCTAssertEqual(coordinator->identifier(), "");

    auto const timeline = proc::timeline::make_shared();

    coordinator->set_timeline(timeline, "1");

    XCTAssertEqual(coordinator->timeline(), timeline);
    XCTAssertEqual(coordinator->identifier(), "1");

    XCTAssertEqual(called_set_container.size(), 1);
    XCTAssertEqual(called_set_container.at(0)->identifier(), "1");
    XCTAssertEqual(called_set_container.at(0)->timeline(), timeline);
    XCTAssertEqual(called_set_container.at(0)->sample_rate(), 44100);

    XCTAssertEqual(called_set_identifier.size(), 1);
    XCTAssertEqual(called_set_identifier.at(0), "1");

    coordinator->reset_timeline();

    XCTAssertEqual(called_set_container.size(), 2);
    XCTAssertEqual(called_set_container.at(1)->identifier(), "");
    XCTAssertEqual(called_set_container.at(1)->timeline(), std::nullopt);
    XCTAssertEqual(called_set_container.at(1)->sample_rate(), 44100);

    XCTAssertEqual(called_set_identifier.size(), 2);
    XCTAssertEqual(called_set_identifier.at(1), "");
}

- (void)test_set_channel_mapping {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<channel_mapping> called;

    self->_cpp.player->set_ch_mapping_handler = [&called](channel_mapping const &ch_mapping) {
        called.emplace_back(ch_mapping);
    };

    auto const ch_mapping = channel_mapping{.indices = {3, 2, 1}};

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

    std::vector<std::pair<std::optional<channel_index_t>, fragment_range>> called;

    self->_cpp.player->overwrite_handler = [&called](std::optional<channel_index_t> file_ch_idx,
                                                     fragment_range frag_range) {
        called.emplace_back(file_ch_idx, frag_range);
    };

    playing::configuration configuration{.sample_rate = 4};
    self->_cpp.renderer->configuration_handler = [&configuration] { return configuration; };

    coordinator->overwrite(proc::time::range{0, 4});

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second.index, 0);
    XCTAssertEqual(called.at(0).second.length, 1);

    called.clear();

    coordinator->overwrite(proc::time::range{0, 5});

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second.index, 0);
    XCTAssertEqual(called.at(0).second.length, 2);

    called.clear();

    coordinator->overwrite(proc::time::range{-1, 6});

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second.index, -1);
    XCTAssertEqual(called.at(0).second.length, 3);
}

- (void)test_identifier {
    auto const coordinator = self->_cpp.setup_coordinator();

    std::vector<std::string> called_exporter_identifier;
    std::vector<std::string> called_player_identifier;

    playing::configuration configuration{.sample_rate = 4};
    self->_cpp.renderer->configuration_handler = [&configuration] { return configuration; };
    self->_cpp.exporter->set_timeline_container_handler =
        [&called_exporter_identifier](timeline_container_ptr container) {
            called_exporter_identifier.emplace_back(container->identifier());
        };
    self->_cpp.player->set_identifier_handler = [&called_player_identifier](std::string identifier) {
        called_player_identifier.emplace_back(identifier);
    };

    XCTAssertEqual(coordinator->identifier(), "");

    auto const timeline = proc::timeline::make_shared();

    coordinator->set_timeline(timeline, "1");

    XCTAssertEqual(coordinator->identifier(), "1");
    XCTAssertEqual(called_exporter_identifier.size(), 1);
    XCTAssertEqual(called_exporter_identifier.at(0), "1");
    XCTAssertEqual(called_player_identifier.size(), 1);
    XCTAssertEqual(called_player_identifier.at(0), "1");
}

- (void)test_channel_mapping {
    auto const coordinator = self->_cpp.setup_coordinator();

    channel_mapping const ch_mapping{.indices = {2, 3}};

    self->_cpp.player->ch_mapping_handler = [&ch_mapping] { return ch_mapping; };

    XCTAssertEqual(coordinator->channel_mapping(), (channel_mapping{.indices = {2, 3}}));
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

- (void)test_configuration {
    auto const coordinator = self->_cpp.setup_coordinator();

    playing::configuration configuration{
        .sample_rate = 1000, .pcm_format = audio::pcm_format::int16, .channel_count = 1};
    self->_cpp.renderer->configuration_handler = [&configuration] { return configuration; };

    XCTAssertEqual(coordinator->configuration().sample_rate, 1000);
    XCTAssertEqual(coordinator->configuration().pcm_format, audio::pcm_format::int16);
    XCTAssertEqual(coordinator->configuration().channel_count, 1);

    configuration.sample_rate = 2000;
    configuration.pcm_format = audio::pcm_format::float32;
    configuration.channel_count = 2;

    XCTAssertEqual(coordinator->configuration().sample_rate, 2000);
    XCTAssertEqual(coordinator->configuration().pcm_format, audio::pcm_format::float32);
    XCTAssertEqual(coordinator->configuration().channel_count, 2);
}

- (void)test_observe_configuration {
    auto const coordinator = self->_cpp.setup_coordinator();
    observing::canceller_pool pool;

    std::vector<timeline_container_ptr> called_containers;

    self->_cpp.exporter->set_timeline_container_handler = [&called_containers](timeline_container_ptr container) {
        called_containers.emplace_back(container);
    };

    // configuration_chainとは別で返す（実際は同じ値になる）
    playing::configuration configuration{.sample_rate = 5};
    self->_cpp.renderer->configuration_handler = [&configuration] { return configuration; };

    std::vector<playing::configuration> called_configrations;

    coordinator
        ->observe_configuration(
            [&called_configrations](auto const &config) { called_configrations.emplace_back(config); })
        .sync()
        ->add_to(pool);

    XCTAssertEqual(called_configrations.size(), 1);
    XCTAssertEqual(called_configrations.at(0), (playing::configuration{}));
    XCTAssertEqual(called_containers.size(), 0);

    self->_cpp.configulation_holder->set_value(
        {.sample_rate = 4, .pcm_format = audio::pcm_format::float32, .channel_count = 1});

    XCTAssertEqual(called_configrations.size(), 2);
    XCTAssertEqual(
        called_configrations.at(1),
        (playing::configuration{.sample_rate = 4, .pcm_format = audio::pcm_format::float32, .channel_count = 1}));
    XCTAssertEqual(called_containers.size(), 1);
    XCTAssertEqual(called_containers.at(0)->sample_rate(), 5);
}

- (void)test_is_playing_chain {
    auto const coordinator = self->_cpp.setup_coordinator();
    observing::canceller_pool pool;

    auto const is_playing = observing::value::holder<bool>::make_shared(false);

    self->_cpp.player->observe_is_playing_handler = [&is_playing](auto &&handler) {
        return is_playing->observe(std::move(handler));
    };

    std::vector<bool> called;

    coordinator->observe_is_playing([&called](bool const &is_playing) { called.emplace_back(is_playing); })
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

    playing::configuration configuration{.sample_rate = 4};
    self->_cpp.renderer->configuration_handler = [&configuration] { return configuration; };

    std::vector<std::pair<std::optional<channel_index_t>, fragment_range>> called;

    self->_cpp.player->overwrite_handler = [&called](std::optional<channel_index_t> ch_idx, fragment_range frag_range) {
        called.emplace_back(std::make_pair(ch_idx, frag_range));
    };

    self->_cpp.exporter_event_notifier->notify(
        exporter_event{.result = exporter_result_t{exporter_method::export_ended}, .range = proc::time::range{0, 1}});

    XCTAssertEqual(called.size(), 1);
    XCTAssertEqual(called.at(0).first, std::nullopt);
    XCTAssertEqual(called.at(0).second.index, 0);
    XCTAssertEqual(called.at(0).second.length, 1);

    self->_cpp.exporter_event_notifier->notify(
        exporter_event{.result = exporter_result_t{exporter_method::export_ended}, .range = proc::time::range{-1, 6}});

    XCTAssertEqual(called.size(), 2);
    XCTAssertEqual(called.at(1).first, std::nullopt);
    XCTAssertEqual(called.at(1).second.index, -1);
    XCTAssertEqual(called.at(1).second.length, 3);
}

@end
