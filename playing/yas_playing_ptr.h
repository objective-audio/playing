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
class channel_mapping;

class playable;
class renderable;
class coordinator_renderable;
class buffering_element_protocol;
class buffering_channel_protocol;
class buffering_resource_protocol;
class reading_resource_protocol;
class player_resource_protocol;
class exportable;

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
using channel_mapping_ptr = std::shared_ptr<channel_mapping>;

using playable_ptr = std::shared_ptr<playable>;
using renderable_ptr = std::shared_ptr<renderable>;
using coordinator_renderable_ptr = std::shared_ptr<coordinator_renderable>;
using buffering_element_protocol_ptr = std::shared_ptr<buffering_element_protocol>;
using buffering_channel_protocol_ptr = std::shared_ptr<buffering_channel_protocol>;
using buffering_resource_protocol_ptr = std::shared_ptr<buffering_resource_protocol>;
using reading_resource_protocol_ptr = std::shared_ptr<reading_resource_protocol>;
using player_resource_protocol_ptr = std::shared_ptr<player_resource_protocol>;
using exportable_ptr = std::shared_ptr<exportable>;
}  // namespace yas::playing
