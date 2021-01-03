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
    audio::io_device_ptr device;
    std::shared_ptr<sample::controller> controller{nullptr};

    chaining::value::holder_ptr<bool> is_playing = chaining::value::holder<bool>::make_shared(false);
    chaining::value::holder_ptr<configuration> config = chaining::value::holder<configuration>::make_shared(
        configuration{.sample_rate = 0, .pcm_format = audio::pcm_format::other, .channel_count = 0});

    chaining::observer_pool pool;

    view_controller_cpp() {
        auto const session = audio::ios_session::shared();
        this->device = audio::ios_device::make_renewable_device(session);
        auto result = session->activate();
        this->controller = sample::controller::make_shared(this->device);
    }
};
}

@interface ViewController ()

@property (nonatomic, weak) IBOutlet UIButton *playButton;
@property (nonatomic, weak) IBOutlet UIButton *minusButton;
@property (nonatomic, weak) IBOutlet UIButton *plusButton;
@property (nonatomic, weak) IBOutlet UISlider *frequencySlider;
@property (nonatomic, weak) IBOutlet UILabel *playFrameLabel;
@property (nonatomic, weak) IBOutlet UILabel *configurationLabel;
@property (nonatomic, weak) IBOutlet UILabel *stateLabel;
@property (nonatomic, weak) IBOutlet UILabel *frequencyLabel;

@property (nonatomic) CADisplayLink *frameDisplayLink;
@property (nonatomic) CADisplayLink *statusDisplayLink;

@end

@implementation ViewController {
    sample::view_controller_cpp _cpp;
}

- (void)dealloc {
    [self.frameDisplayLink invalidate];
    [self.statusDisplayLink invalidate];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    [self.minusButton setTitle:@"minus1s" forState:UIControlStateNormal];
    [self.plusButton setTitle:@"plus1s" forState:UIControlStateNormal];

    auto unowned_self = objc_ptr_with_move_object([[YASUnownedObject<ViewController *> alloc] initWithObject:self]);

    auto const &controller = self->_cpp.controller;
    auto &pool = self->_cpp.pool;

    self.frequencySlider.value = controller->frequency->raw();

    controller->coordinator->is_playing_chain().send_to(self->_cpp.is_playing).sync()->add_to(pool);
    controller->coordinator->configuration_chain().send_to(self->_cpp.config).sync()->add_to(pool);

    controller->frequency->chain()
        .perform([unowned_self](float const &) {
            ViewController *viewController = [unowned_self.object() object];
            [viewController _updateFrequencyLabel];
        })
        .sync()
        ->add_to(pool);

    self->_cpp.is_playing->chain()
        .perform([unowned_self](bool const &is_playing) {
            NSString *title = is_playing ? @"Stop" : @"Play";
            ViewController *viewController = [unowned_self.object() object];
            [viewController.playButton setTitle:title forState:UIControlStateNormal];
        })
        .sync()
        ->add_to(pool);

    self->_cpp.config->chain()
        .perform([unowned_self](auto const &) {
            ViewController *viewController = [unowned_self.object() object];
            [viewController _updateConfigurationLabel];
        })
        .sync()
        ->add_to(pool);

    self.frameDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(_updatePlayFrame:)];
    self.frameDisplayLink.preferredFramesPerSecond = 30;
    [self.frameDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];

    self.statusDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(_updateStateLabel:)];
    self.statusDisplayLink.preferredFramesPerSecond = 10;
    [self.statusDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
}

- (IBAction)playButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator->set_playing(!coordinator->is_playing());
}

- (IBAction)minusButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator->seek(coordinator->current_frame() - coordinator->sample_rate());
}

- (IBAction)plusButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator->seek(coordinator->current_frame() + coordinator->sample_rate());
}

- (IBAction)frequencyChanged:(UISlider *)sender {
    self->_cpp.controller->frequency->set_value(sender.value);
}

- (void)_updateConfigurationLabel {
    std::vector<std::string> texts;

    auto const &coordinator = self->_cpp.controller->coordinator;
    texts.emplace_back("sample rate : " + std::to_string(coordinator->sample_rate()));
    texts.emplace_back("channel count : " + std::to_string(coordinator->channel_count()));
    texts.emplace_back("pcm format : " + to_string(coordinator->pcm_format()));

    std::string text = joined(texts, "\n");

    self.configurationLabel.text = (__bridge NSString *)to_cf_object(text);
}

- (void)_updateStateLabel:(CADisplayLink *)displayLink {
    std::vector<std::string> ch_texts;
    self.stateLabel.text = (__bridge NSString *)to_cf_object(joined(ch_texts, "\n"));
}

- (void)_updatePlayFrame:(CADisplayLink *)displayLink {
    std::string const play_frame_str =
        "play_frame : " + std::to_string(self->_cpp.controller->coordinator->current_frame());
    self.playFrameLabel.text = (__bridge NSString *)to_cf_object(play_frame_str);
}

- (void)_updateFrequencyLabel {
    self.frequencyLabel.text = [NSString stringWithFormat:@"%.1fHz", self->_cpp.controller->frequency->raw()];
}

@end
