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

    chaining::observer_pool pool;
};
}

@interface ViewController ()

@property (nonatomic, retain) IBOutlet UIButton *playButton;

@end

@implementation ViewController {
    sample::view_controller_cpp _cpp;
}

- (void)dealloc {
    [_playButton release];
    [super dealloc];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    auto unowned_self = make_objc_ptr([[YASUnownedObject<ViewController *> alloc] initWithObject:self]);

    auto &controller = self->_cpp.controller;

    controller->pool += controller->coordinator.is_playing_chain().send_to(self->_cpp.is_playing).sync();

    controller->pool += self->_cpp.is_playing.chain()
                            .perform([unowned_self](bool const &is_playing) {
                                NSString *title = is_playing ? @"Stop" : @"Play";
                                ViewController *viewController = [unowned_self.object() object];
                                [viewController.playButton setTitle:title forState:UIControlStateNormal];
                            })
                            .sync();
}

- (void)set_controller:(std::shared_ptr<yas::playing::sample::controller>)controller {
    self->_cpp.controller = std::move(controller);
}

- (IBAction)playButtonTapped:(UIButton *)sender {
    auto &coordinator = self->_cpp.controller->coordinator;
    coordinator.set_playing(!coordinator.is_playing());
}

@end
