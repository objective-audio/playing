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
    playing::state_map_vector_holder_t playing_buffer_state;

    chaining::observer_pool pool;
};
}

@interface ViewController ()

@property (nonatomic, retain) IBOutlet UIButton *playButton;
@property (nonatomic, retain) IBOutlet UILabel *stateLabel;

@end

@implementation ViewController {
    sample::view_controller_cpp _cpp;
}

- (void)dealloc {
    [_playButton release];
    [_stateLabel release];
    [super dealloc];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    auto unowned_self = make_objc_ptr([[YASUnownedObject<ViewController *> alloc] initWithObject:self]);

    auto &controller = self->_cpp.controller;

    controller->pool += controller->coordinator.is_playing_chain().send_to(self->_cpp.is_playing).sync();
    controller->pool += controller->coordinator.state_chain().send_to(self->_cpp.playing_buffer_state).sync();

    controller->pool += self->_cpp.is_playing.chain()
                            .perform([unowned_self](bool const &is_playing) {
                                NSString *title = is_playing ? @"Stop" : @"Play";
                                ViewController *viewController = [unowned_self.object() object];
                                [viewController.playButton setTitle:title forState:UIControlStateNormal];
                            })
                            .sync();
    controller->pool += self->_cpp.playing_buffer_state.chain()
                            .perform([unowned_self](auto const &) {
                                ViewController *viewController = [unowned_self.object() object];
                                [viewController _update_state_label];
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

- (void)_update_state_label {
    std::vector<std::string> ch_texts;

    auto &coordinator = self->_cpp.controller->coordinator;
    ch_texts.emplace_back("sample rate : " + std::to_string(coordinator.sample_rate()));
    ch_texts.emplace_back("channel count : " + std::to_string(coordinator.channel_count()));
    ch_texts.emplace_back("pcm format : " + to_string(coordinator.pcm_format()));
    ch_texts.emplace_back("");

    auto &state = self->_cpp.playing_buffer_state;
    channel_index_t ch_idx = 0;

    for (auto &ch_state : state.raw()) {
        std::vector<std::string> buf_texts;
        buf_texts.emplace_back("ch : " + std::to_string(ch_idx++));
        for (auto &buf_pair : ch_state.raw()) {
            auto buf_idx_str = std::to_string(buf_pair.first);
            auto &buf_state = buf_pair.second;
            auto frag_idx_str = buf_state.frag_idx ? std::to_string(*buf_state.frag_idx) : "-";
            auto frag_kind_str = to_string(buf_state.kind);
            buf_texts.emplace_back("  " + buf_idx_str + " : " + frag_idx_str + " : " + frag_kind_str);
        }
        ch_texts.emplace_back(buf_texts.size() > 0 ? joined(buf_texts, "\n") : "empty");
    }

    std::string text = joined(ch_texts, "\n");

    self.stateLabel.text = (NSString *)to_cf_object(text);
}

@end
