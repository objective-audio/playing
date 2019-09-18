//
//  yas_playing_ptr.h
//

#pragma once

#include <memory>

namespace yas::playing {
class timeline_exporter;
class timeline_container;
class audio_coordinator;
class timeline_cancel_matcher;
class timeline_range_cancel_request;
class audio_renderer;
class audio_player;
class cancel_id;

class audio_renderable;

using timeline_exporter_ptr = std::shared_ptr<timeline_exporter>;
using timeline_exporter_wptr = std::weak_ptr<timeline_exporter>;
using timeline_container_ptr = std::shared_ptr<timeline_container>;
using audio_coordinator_ptr = std::shared_ptr<audio_coordinator>;
using timeline_cancel_matcher_ptr = std::shared_ptr<timeline_cancel_matcher>;
using timeline_range_cancel_request_ptr = std::shared_ptr<timeline_range_cancel_request>;
using audio_renderer_ptr = std::shared_ptr<audio_renderer>;
using audio_player_ptr = std::shared_ptr<audio_player>;
using cancel_id_ptr = std::shared_ptr<cancel_id>;

using audio_renderable_ptr = std::shared_ptr<audio_renderable>;
}  // namespace yas::playing
