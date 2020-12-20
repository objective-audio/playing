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
class audio_player;
class cancel_id;
class audio_player_rendering;
class audio_circular_buffer;
class audio_buffering;
class audio_buffering_channel;
class audio_buffering_element;
class audio_reading;
class audio_rendering;
class background_queue;

class audio_playable;
class audio_renderable;
class audio_coordinator_renderable;
class audio_buffering_element_protocol;
class audio_buffering_channel_protocol;
class audio_buffering_protocol;
class audio_reading_protocol;
class audio_rendering_protocol;

using timeline_exporter_ptr = std::shared_ptr<timeline_exporter>;
using timeline_exporter_wptr = std::weak_ptr<timeline_exporter>;
using timeline_container_ptr = std::shared_ptr<timeline_container>;
using audio_coordinator_ptr = std::shared_ptr<audio_coordinator>;
using timeline_cancel_matcher_ptr = std::shared_ptr<timeline_cancel_matcher>;
using timeline_range_cancel_request_ptr = std::shared_ptr<timeline_range_cancel_request>;
using audio_renderer_ptr = std::shared_ptr<audio_renderer>;
using audio_player_ptr = std::shared_ptr<audio_player>;
using player_ptr = std::shared_ptr<audio_player>;
using cancel_id_ptr = std::shared_ptr<cancel_id>;
using audio_player_rendering_ptr = std::shared_ptr<audio_player_rendering>;
using audio_circular_buffer_ptr = std::shared_ptr<audio_circular_buffer>;
using audio_circular_buffer_wptr = std::weak_ptr<audio_circular_buffer>;
using audio_buffering_ptr = std::shared_ptr<audio_buffering>;
using audio_buffering_channel_ptr = std::shared_ptr<audio_buffering_channel>;
using audio_buffering_element_ptr = std::shared_ptr<audio_buffering_element>;
using audio_reading_ptr = std::shared_ptr<audio_reading>;
using audio_rendering_ptr = std::shared_ptr<audio_rendering>;

using audio_playable_ptr = std::shared_ptr<audio_playable>;
using audio_renderable_ptr = std::shared_ptr<audio_renderable>;
using audio_coordinator_renderable_ptr = std::shared_ptr<audio_coordinator_renderable>;
using audio_buffering_element_protocol_ptr = std::shared_ptr<audio_buffering_element_protocol>;
using audio_buffering_channel_protocol_ptr = std::shared_ptr<audio_buffering_channel_protocol>;
using audio_buffering_protocol_ptr = std::shared_ptr<audio_buffering_protocol>;
using audio_reading_protocol_ptr = std::shared_ptr<audio_reading_protocol>;
using audio_rendering_protocol_ptr = std::shared_ptr<audio_rendering_protocol>;
}  // namespace yas::playing
