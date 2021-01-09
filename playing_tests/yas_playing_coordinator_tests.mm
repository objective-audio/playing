//
//  yas_playing_coordinator_tests.mm
//

#import <XCTest/XCTest.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

namespace yas::playing::test {
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
    std::shared_ptr<test::worker> worker = nullptr;
    std::shared_ptr<test::renderer> renderer = nullptr;
    std::shared_ptr<test::player> player = nullptr;
    std::shared_ptr<test::exporter> exporter = nullptr;

    coordinator_ptr coordinator = nullptr;

    void setup() {
        this->worker = std::make_shared<test::worker>();
        this->renderer = std::make_shared<test::renderer>();
        this->player = std::make_shared<test::player>();
        this->exporter = std::make_shared<test::exporter>();
        this->coordinator =
            coordinator::make_shared(test::identifier, this->worker, this->renderer, this->player, this->exporter);
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
    test::coordinator_cpp _cpp;
}

- (void)tearDown {
    self->_cpp.reset();

    [super tearDown];
}

- (void)test_set_timeline {
#warning todo
}

- (void)test_set_channel_mapping {
#warning todo
}

- (void)test_set_playing {
#warning todo
}

- (void)test_seek {
#warning todo
}

- (void)test_overwrite {
#warning todo
}

- (void)test_identifier {
#warning todo
}

- (void)test_timeline {
#warning todo
}

- (void)test_channel_mapping {
#warning todo
}

- (void)test_is_playing {
#warning todo
}

- (void)test_current_frame {
#warning todo
}

- (void)test_sample_rate {
#warning todo
}

- (void)test_pcm_format {
#warning todo
}

- (void)test_channel_count {
#warning todo
}

- (void)test_configuration_chain {
#warning todo
}

- (void)test_is_playing_chain {
#warning todo
}

@end
