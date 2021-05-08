//
//  yas_playing_ptr.h
//

#pragma once

#include <memory>

namespace yas::playing {
class exporter;
class exporter_resource;
class timeline_container;
class timeline_cancel_matcher;
class timeline_range_cancel_request;
class cancel_id;
class coordinator;
class renderer;
class player;
class buffering_resource;
class buffering_channel;
class buffering_element;
class reading_resource;
class player_resource;

class coordinator_player_interface;
class coordinator_renderer_interface;
class buffering_element_interface;
class buffering_channel_interface;
class buffering_resource_interface;
class reading_resource_interface;
class player_resource_interface;
class coordinator_exporter_interface;

using exporter_ptr = std::shared_ptr<exporter>;
using exporter_resource_ptr = std::shared_ptr<exporter_resource>;
using timeline_container_ptr = std::shared_ptr<timeline_container>;
using coordinator_ptr = std::shared_ptr<coordinator>;
using timeline_cancel_matcher_ptr = std::shared_ptr<timeline_cancel_matcher>;
using timeline_range_cancel_request_ptr = std::shared_ptr<timeline_range_cancel_request>;
using renderer_ptr = std::shared_ptr<renderer>;
using player_ptr = std::shared_ptr<player>;
using cancel_id_ptr = std::shared_ptr<cancel_id>;
using buffering_resource_ptr = std::shared_ptr<buffering_resource>;
using buffering_channel_ptr = std::shared_ptr<buffering_channel>;
using buffering_element_ptr = std::shared_ptr<buffering_element>;
using reading_resource_ptr = std::shared_ptr<reading_resource>;
using player_resource_ptr = std::shared_ptr<player_resource>;

using buffering_element_protocol_ptr = std::shared_ptr<buffering_element_interface>;
using buffering_channel_protocol_ptr = std::shared_ptr<buffering_channel_interface>;
using reading_resource_protocol_ptr = std::shared_ptr<reading_resource_interface>;
using exportable_ptr = std::shared_ptr<coordinator_exporter_interface>;
}  // namespace yas::playing
