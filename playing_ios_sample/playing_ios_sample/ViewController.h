//
//  ViewController.h
//

#import <UIKit/UIKit.h>
#import <memory>

namespace yas::playing::sample {
struct controller;
}

@interface ViewController : UIViewController

- (void)set_controller:(std::shared_ptr<yas::playing::sample::controller>)controller;

@end
