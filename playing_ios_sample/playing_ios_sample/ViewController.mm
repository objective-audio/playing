//
//  ViewController.m
//

#import "ViewController.h"
#import "yas_playing_sample_controller.hpp"

using namespace yas;
using namespace yas::playing;

namespace yas::playing::sample {
struct view_controller_cpp {
    std::shared_ptr<sample::controller> controller{nullptr};
};
}

@interface ViewController ()

@end

@implementation ViewController {
    sample::view_controller_cpp _cpp;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)set_controller:(std::shared_ptr<yas::playing::sample::controller>)controller {
    self->_cpp.controller = std::move(controller);
}

@end
