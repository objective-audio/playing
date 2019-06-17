//
//  ViewController.m
//

#import "ViewController.h"
#import <cpp_utils/yas_objc_ptr.h>
#import <objc_utils/yas_objc_unowned.h>
#import "yas_playing_sample_controller.hpp"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::sample {
struct view_controller_cpp {
    std::shared_ptr<sample::controller> controller{nullptr};

    chaining::value::holder<bool> is_playing{false};
    chaining::value::holder<audio_configuration> configuration{
        audio_configuration{.sample_rate = 0, .pcm_format = audio::pcm_format::other, .channel_count = 0}};
    playing::state_map_vector_holder_t playing_buffer_state;

    chaining::observer_pool pool;

    objc_ptr<CADisplayLink *> display_link;
};
}

@interface ViewController ()

@property (nonatomic, retain) IBOutlet UIButton *playButton;
@property (nonatomic, retain) IBOutlet UIButton *minusButton;
@property (nonatomic, retain) IBOutlet UIButton *plusButton;
@property (nonatomic, retain) IBOutlet UILabel *playFrameLabel;
@property (nonatomic, retain) IBOutlet UILabel *configurationLabel;
@property (nonatomic, retain) IBOutlet UILabel *stateLabel;

@end

@implementation ViewController {
    sample::view_controller_cpp _cpp;
}

- (void)dealloc {
    [_playButton release];
    [_minusButton release];
    [_plusButton release];
    [_playFrameLabel release];
    [_configurationLabel release];
    [_stateLabel release];
    [super dealloc];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    [self.minusButton setTitle:@"minus1s" forState:UIControlStateNormal];
    [self.plusButton setTitle:@"plus1s" forState:UIControlStateNormal];

    auto unowned_self = make_objc_ptr([[YASUnownedObject<ViewController *> alloc] initWithObject:self]);

    auto &controller = self->_cpp.controller;

    controller->pool += controller->coordinator.chain_is_playing().send_to(self->_cpp.is_playing).sync();
    controller->pool += controller->coordinator.chain_configuration().send_to(self->_cpp.configuration).sync();
    controller->pool += controller->coordinator.chain_state().send_to(self->_cpp.playing_buffer_state).sync();

    controller->pool += self->_cpp.is_playing.chain()
                            .perform([unowned_self](bool const &is_playing) {
                                NSString *title = is_playing ? @"Stop" : @"Play";
                                ViewController *viewController = [unowned_self.object() object];
                                [viewController.playButton setTitle:title forState:UIControlStateNormal];
                            })
                            .sync();
    controller->pool += self->_cpp.configuration.chain()
                            .perform([unowned_self](auto const &) {
                                ViewController *viewController = [unowned_self.object() object];
                                [viewController _update_configuration_label];
                            })
                            .sync();
    controller->pool += self->_cpp.playing_buffer_state.chain()
                            .perform([unowned_self](auto const &) {
                                ViewController *viewController = [unowned_self.object() object];
                                [viewController _update_state_label];
                            })
                            .sync();

    self->_cpp.display_link = make_objc_ptr<CADisplayLink *>([self]() {
        CADisplayLink *displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(update_play_frame:)];
        displayLink.preferredFramesPerSecond = 30;
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        return displayLink;
    });
}

- (void)set_controller:(std::shared_ptr<yas::playing::sample::controller>)controller {
    self->_cpp.controller = std::move(controller);
}

- (IBAction)playButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator.set_playing(!coordinator.is_playing());
}

- (IBAction)minusButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator.seek(coordinator.play_frame() - coordinator.sample_rate());
}

- (IBAction)plusButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator.seek(coordinator.play_frame() + coordinator.sample_rate());
}

- (void)_update_configuration_label {
    std::vector<std::string> texts;

    auto &coordinator = self->_cpp.controller->coordinator;
    texts.emplace_back("sample rate : " + std::to_string(coordinator.sample_rate()));
    texts.emplace_back("channel count : " + std::to_string(coordinator.channel_count()));
    texts.emplace_back("pcm format : " + to_string(coordinator.pcm_format()));

    std::string text = joined(texts, "\n");

    self.configurationLabel.text = (NSString *)to_cf_object(text);
}

- (void)_update_state_label {
    std::vector<std::string> ch_texts;

    auto &state = self->_cpp.playing_buffer_state;
    channel_index_t ch_idx = 0;

    for (auto &ch_state : state.raw()) {
        std::vector<std::string> buf_texts;
        buf_texts.emplace_back("channel : " + std::to_string(ch_idx++));
        for (auto &buf_pair : ch_state.raw()) {
            auto buf_idx_str = std::to_string(buf_pair.first);
            auto &buf_state = buf_pair.second;
            auto frag_idx_str = buf_state.frag_idx ? std::to_string(*buf_state.frag_idx) : "-";
            auto frag_kind_str = to_string(buf_state.kind);
            buf_texts.emplace_back("  buffer:" + buf_idx_str + ", fragment:" + frag_idx_str +
                                   ", kind:" + frag_kind_str);
        }
        ch_texts.emplace_back(buf_texts.size() > 0 ? joined(buf_texts, "\n") : "empty");
    }

    self.stateLabel.text = (NSString *)to_cf_object(joined(ch_texts, "\n"));
}

- (void)update_play_frame:(CADisplayLink *)displayLink {
    std::string const play_frame_str =
        "play_frame : " + std::to_string(self->_cpp.controller->coordinator.play_frame());
    self.playFrameLabel.text = (NSString *)to_cf_object(play_frame_str);
}
@end
