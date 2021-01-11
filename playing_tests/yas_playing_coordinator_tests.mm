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

    std::vector<std::pair<std::optional<channel_index_t>, fragment_range>> called;

    self->_cpp.player->overwrite_handler = [&called](std::optional<channel_index_t> file_ch_idx,
                                                     fragment_range frag_range) {
        called.emplace_back(file_ch_idx, frag_range);
    };

    self->_cpp.renderer->sample_rate_handler = [] { return 4; };

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
