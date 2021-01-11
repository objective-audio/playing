//
//  yas_playing_renderer_tests.mm
//

#import <XCTest/XCTest.h>
#import <audio/audio.h>
#import <playing/playing.h>

using namespace yas;
using namespace yas::playing;

namespace yas::playing::renderer_test {
struct io_core : audio::io_core {
    void set_render_handler(std::optional<audio::io_render_f>) {
    }

    void set_maximum_frames_per_slice(uint32_t const) {
    }

    bool start() {
        return true;
    }

    void stop() {
    }
};

struct device : audio::io_device {
    std::function<std::optional<audio::format>(void)> input_format_handler;
    std::function<std::optional<audio::format>(void)> output_format_handler;
    chaining::notifier_ptr<io_device::method> const notifier = chaining::notifier<io_device::method>::make_shared();

    std::optional<audio::format> input_format() const override {
        return this->input_format_handler();
    }

    std::optional<audio::format> output_format() const override {
        return this->output_format_handler();
    }

    audio::io_core_ptr make_io_core() const override {
        return std::make_shared<io_core>();
    }

    std::optional<audio::interruptor_ptr> const &interruptor() const override {
        static std::optional<audio::interruptor_ptr> _interruptor = std::nullopt;
        return _interruptor;
    }

    chaining::chain_unsync_t<io_device::method> io_device_chain() override {
        return this->notifier->chain();
    }
};
}

@interface yas_playing_renderer_tests : XCTestCase

@end

@implementation yas_playing_renderer_tests {
}

- (void)test_constructor_with_format {
    auto const device = std::make_shared<renderer_test::device>();

    std::optional<audio::format> output_format = audio::format{{.sample_rate = 1000, .channel_count = 2}};

    device->output_format_handler = [&output_format] { return output_format; };

    auto const renderer = playing::renderer::make_shared(device);

    XCTAssertEqual(renderer->sample_rate(), 1000);
    XCTAssertEqual(renderer->channel_count(), 2);
}

- (void)test_constructor_null_format {
    auto const device = std::make_shared<renderer_test::device>();

    std::optional<audio::format> output_format = std::nullopt;

    device->output_format_handler = [&output_format] { return output_format; };

    auto const renderer = playing::renderer::make_shared(device);

    XCTAssertEqual(renderer->sample_rate(), 0);
    XCTAssertEqual(renderer->channel_count(), 0);
}

@end
