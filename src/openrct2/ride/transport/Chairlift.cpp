#pragma region Copyright (c) 2014-2017 OpenRCT2 Developers
/*****************************************************************************
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * OpenRCT2 is the work of many authors, a full list can be found in contributors.md
 * For more information, visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#include "../../interface/viewport.h"
#include "../../paint/Paint.h"
#include "../../paint/Supports.h"
#include "../../world/map.h"
#include "../Ride.h"
#include "../ride_data.h"
#include "../Track.h"
#include "../track_paint.h"

enum
{
    SPR_CHAIRLIFT_CABLE_FLAT_SW_NE = 20500,
    SPR_CHAIRLIFT_CABLE_FLAT_SE_NW,
    SPR_20502,
    SPR_20503,
    SPR_20504,
    SPR_20505,
    SPR_CHAIRLIFT_STATION_COLUMN_NE_SW,
    SPR_CHAIRLIFT_STATION_COLUMN_SE_NW,
    SPR_20508,
    SPR_20509,
    SPR_20510,
    SPR_20511,
    SPR_20512,
    SPR_20513,
    SPR_20514,
    SPR_20515,
    SPR_CHAIRLIFT_CABLE_UP_SW_NE,
    SPR_CHAIRLIFT_CABLE_UP_NW_SE,
    SPR_CHAIRLIFT_CABLE_UP_NE_SW,
    SPR_CHAIRLIFT_CABLE_UP_SE_NW,
    SPR_20520,
    SPR_20521,
    SPR_20522,
    SPR_20523,
    SPR_20524,
    SPR_20525,
    SPR_20526,
    SPR_20527,
    SPR_CHAIRLIFT_CORNER_NW_SW,
    SPR_CHAIRLIFT_CORNER_NW_NE,
    SPR_CHAIRLIFT_CORNER_SE_NE,
    SPR_CHAIRLIFT_CORNER_SW_SE,
    SPR_20532,
    SPR_20533,
    SPR_20534,
    SPR_20535,
    SPR_20536,
    SPR_20537,
    SPR_20538,
    SPR_20539,
    SPR_CHAIRLIFT_BULLWHEEL_FRAME_1,
    SPR_CHAIRLIFT_BULLWHEEL_FRAME_2,
    SPR_CHAIRLIFT_BULLWHEEL_FRAME_3,
    SPR_CHAIRLIFT_BULLWHEEL_FRAME_4,
    SPR_CHAIRLIFT_STATION_END_CAP_NE,
    SPR_CHAIRLIFT_STATION_END_CAP_SE,
    SPR_CHAIRLIFT_STATION_END_CAP_SW,
    SPR_CHAIRLIFT_STATION_END_CAP_NW,
};

const uint32 chairlift_bullwheel_frames[] = { SPR_CHAIRLIFT_BULLWHEEL_FRAME_1, SPR_CHAIRLIFT_BULLWHEEL_FRAME_2,
                                              SPR_CHAIRLIFT_BULLWHEEL_FRAME_3, SPR_CHAIRLIFT_BULLWHEEL_FRAME_4 };

static void chairlift_paint_util_draw_supports(paint_session * session, sint32 segments, uint16 height)
{
    bool success = false;

    for (sint32 s = 0; s < 9; s++)
    {
        if (!(segments & segment_offsets[s]))
        {
            continue;
        }

        if (metal_a_supports_paint_setup(session, METAL_SUPPORTS_TRUSS, s, 0, height, session->TrackColours[SCHEME_SUPPORTS]))
        {
            success = true;
        }
    }

    if (success)
    {
        return;
    }

    support_height * supportSegments = session->SupportSegments;
    for (sint32 s = 0; s < 9; s++)
    {
        if (!(segments & segment_offsets[s]))
        {
            continue;
        }
        uint16 temp               = supportSegments[s].height;
        supportSegments[s].height = session->Support.height;
        metal_a_supports_paint_setup(session, METAL_SUPPORTS_TRUSS, s, 0, height, session->TrackColours[SCHEME_SUPPORTS]);
        supportSegments[s].height = temp;
    }
}

static rct_tile_element * chairlift_paint_util_map_get_track_element_at_from_ride_fuzzy(sint32 x, sint32 y, sint32 z,
                                                                                       sint32 rideIndex)
{
    rct_tile_element * tileElement = map_get_first_element_at(x >> 5, y >> 5);
    if (tileElement == NULL)
    {
        return NULL;
    }

    do
    {
        if (tile_element_get_type(tileElement) != TILE_ELEMENT_TYPE_TRACK)
            continue;
        if (track_element_get_ride_index(tileElement) != rideIndex)
            continue;
        if (tileElement->base_height != z && tileElement->base_height != z - 1)
            continue;

        return tileElement;
    } while (!tile_element_is_last_for_tile(tileElement++));

    return NULL;
};

static bool chairlift_paint_util_is_first_track(uint8 rideIndex, const rct_tile_element * tileElement, LocationXY16 pos,
                                                uint8 trackType)
{
    if (track_element_get_type(tileElement) != TRACK_ELEM_BEGIN_STATION)
    {
        return false;
    }

    LocationXY16 delta  = TileDirectionDelta[tile_element_get_direction(tileElement)];
    LocationXY16 newPos = {
        static_cast<sint16>(pos.x - delta.x),
        static_cast<sint16>(pos.y - delta.y),
    };

    rct_tile_element * nextTrack =
        chairlift_paint_util_map_get_track_element_at_from_ride_fuzzy(newPos.x, newPos.y, tileElement->base_height, rideIndex);

    return nextTrack == NULL;
}

static bool chairlift_paint_util_is_last_track(uint8 rideIndex, const rct_tile_element * tileElement, LocationXY16 pos,
                                               uint8 trackType)
{
    if (track_element_get_type(tileElement) != TRACK_ELEM_END_STATION)
    {
        return false;
    }

    LocationXY16 delta  = TileDirectionDelta[tile_element_get_direction(tileElement)];
    LocationXY16 newPos = {
        static_cast<sint16>(pos.x + delta.x),
        static_cast<sint16>(pos.y + delta.y),
    };

    rct_tile_element * nextTrack =
        chairlift_paint_util_map_get_track_element_at_from_ride_fuzzy(newPos.x, newPos.y, tileElement->base_height, rideIndex);

    return nextTrack == NULL;
}

static void chairlift_paint_station_ne_sw(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                          sint32 height, rct_tile_element * tileElement)
{
    const LocationXY16 pos       = session->MapPosition;
    uint8          trackType = track_element_get_type(tileElement);
    Ride *         ride      = get_ride(rideIndex);
    uint32         imageId;

    bool isStart = chairlift_paint_util_is_first_track(rideIndex, tileElement, pos, trackType);
    ;
    bool isEnd = chairlift_paint_util_is_last_track(rideIndex, tileElement, pos, trackType);

    const rct_ride_entrance_definition * entranceStyle = &RideEntranceDefinitions[ride->entrance_style];

    wooden_a_supports_paint_setup(session, 0, 0, height, session->TrackColours[SCHEME_MISC], NULL);

    if (!isStart && !isEnd)
    {
        imageId = ((direction == 0) ? SPR_20502 : SPR_20504) | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());
    }

    imageId = SPR_FLOOR_METAL | session->TrackColours[SCHEME_SUPPORTS];
    sub_98197C(session, imageId, 0, 0, 32, 32, 1, height, 0, 0, height, get_current_rotation());

    bool hasFence = track_paint_util_has_fence(EDGE_NW, pos, tileElement, ride, get_current_rotation());
    if (hasFence)
    {
        imageId = SPR_FENCE_METAL_NW | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 32, 1, 7, height, 0, 2, height + 2, get_current_rotation());
    }
    track_paint_util_draw_station_covers(session, EDGE_NW, hasFence, entranceStyle, direction, height);

    if ((direction == 2 && isStart) || (direction == 0 && isEnd))
    {
        imageId = SPR_FENCE_METAL_NE | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 1, 28, 7, height, 2, 2, height + 4, get_current_rotation());
    }

    hasFence = track_paint_util_has_fence(EDGE_SE, pos, tileElement, ride, get_current_rotation());
    if (hasFence)
    {
        imageId = SPR_FENCE_METAL_SE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 1, 27, height, 0, 30, height + 2, get_current_rotation());
    }
    track_paint_util_draw_station_covers(session, EDGE_SE, hasFence, entranceStyle, direction, height);

    bool drawFrontColumn = true;
    bool drawBackColumn  = true;
    if ((direction == 0 && isStart) || (direction == 2 && isEnd))
    {
        imageId = SPR_FENCE_METAL_SW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 1, 28, 27, height, 30, 2, height + 4, get_current_rotation());

        imageId = chairlift_bullwheel_frames[ride->chairlift_bullwheel_rotation / 16384] | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        imageId = SPR_CHAIRLIFT_STATION_END_CAP_NE | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        drawFrontColumn = false;
    }
    else if ((direction == 2 && isStart) || (direction == 0 && isEnd))
    {
        imageId = chairlift_bullwheel_frames[ride->chairlift_bullwheel_rotation / 16384] | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        imageId = SPR_CHAIRLIFT_STATION_END_CAP_SW | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        drawBackColumn = false;
    }

    if (drawBackColumn)
    {
        imageId = SPR_CHAIRLIFT_STATION_COLUMN_NE_SW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 16, 1, 1, 7, height + 2, 1, 16, height + 2, get_current_rotation());
    }

    if (drawFrontColumn)
    {
        imageId = SPR_CHAIRLIFT_STATION_COLUMN_NE_SW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 30, 16, 1, 1, 7, height + 2, 1, 16, height + 2,
                   get_current_rotation()); // bound offset x is wrong?
    }

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_push_tunnel_left(session, height, TUNNEL_6);
    paint_util_set_general_support_height(session, height + 32, 0x20);
}

static void chairlift_paint_station_se_nw(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                          sint32 height, rct_tile_element * tileElement)
{
    const LocationXY16 pos       = session->MapPosition;
    uint8          trackType = track_element_get_type(tileElement);
    Ride *         ride      = get_ride(rideIndex);
    uint32         imageId;

    bool isStart = chairlift_paint_util_is_first_track(rideIndex, tileElement, pos, trackType);
    ;
    bool isEnd = chairlift_paint_util_is_last_track(rideIndex, tileElement, pos, trackType);

    const rct_ride_entrance_definition * entranceStyle = &RideEntranceDefinitions[ride->entrance_style];

    wooden_a_supports_paint_setup(session, 1, 0, height, session->TrackColours[SCHEME_MISC], NULL);

    if (!isStart && !isEnd)
    {
        imageId = ((direction == 1) ? SPR_20503 : SPR_20505) | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());
    }

    imageId = SPR_FLOOR_METAL | session->TrackColours[SCHEME_SUPPORTS];
    sub_98197C(session, imageId, 0, 0, 32, 32, 1, height, 0, 0, height, get_current_rotation());

    bool hasFence = track_paint_util_has_fence(EDGE_NE, pos, tileElement, ride, get_current_rotation());
    if (hasFence)
    {
        imageId = SPR_FENCE_METAL_NE | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 1, 32, 7, height, 2, 0, height + 2, get_current_rotation());
    }
    track_paint_util_draw_station_covers(session, EDGE_NE, hasFence, entranceStyle, direction, height);

    if ((direction == 1 && isStart) || (direction == 3 && isEnd))
    {
        imageId = SPR_FENCE_METAL_NW | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 28, 1, 7, height, 2, 2, height + 4, get_current_rotation());
    }

    hasFence = track_paint_util_has_fence(EDGE_SW, pos, tileElement, ride, get_current_rotation());
    if (hasFence)
    {
        imageId = SPR_FENCE_METAL_SW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 1, 32, 27, height, 30, 0, height + 2, get_current_rotation());
    }
    track_paint_util_draw_station_covers(session, EDGE_SW, hasFence, entranceStyle, direction, height);

    bool drawRightColumn = true;
    bool drawLeftColumn  = true;
    if ((direction == 1 && isStart) || (direction == 3 && isEnd))
    {
        imageId = chairlift_bullwheel_frames[ride->chairlift_bullwheel_rotation / 16384] | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        imageId = SPR_CHAIRLIFT_STATION_END_CAP_SE | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        drawLeftColumn = false;
    }
    else if ((direction == 3 && isStart) || (direction == 1 && isEnd))
    {
        imageId = SPR_FENCE_METAL_SE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 28, 1, 27, height, 2, 30, height + 4, get_current_rotation());

        imageId = chairlift_bullwheel_frames[ride->chairlift_bullwheel_rotation / 16384] | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        imageId = SPR_CHAIRLIFT_STATION_END_CAP_NW | session->TrackColours[SCHEME_TRACK];
        sub_98199C(session, imageId, 0, 0, 4, 4, 26, height, 14, 14, height + 4, get_current_rotation());

        drawRightColumn = false;
    }

    if (drawLeftColumn)
    {
        imageId = SPR_CHAIRLIFT_STATION_COLUMN_SE_NW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 16, 0, 1, 1, 7, height + 2, 16, 1, height + 2, get_current_rotation());
    }

    if (drawRightColumn)
    {
        imageId = SPR_CHAIRLIFT_STATION_COLUMN_SE_NW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 16, 30, 1, 1, 7, height + 2, 16, 1, height + 2,
                   get_current_rotation()); // bound offset x is wrong?

        paint_util_push_tunnel_right(session, height, TUNNEL_6);
    }

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 32, 0x20);
}

/** rct2: 0x00744068 */
static void chairlift_paint_station(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                    sint32 height, rct_tile_element * tileElement)
{
    if (direction % 2)
    {
        chairlift_paint_station_se_nw(session, rideIndex, trackSequence, direction, height, tileElement);
    }
    else
    {
        chairlift_paint_station_ne_sw(session, rideIndex, trackSequence, direction, height, tileElement);
    }
}

static void chairlift_paint_flat(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction, sint32 height,
                                 rct_tile_element * tileElement)
{
    uint32 imageId;
    if (direction & 1)
    {
        imageId = SPR_CHAIRLIFT_CABLE_FLAT_SE_NW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());
        paint_util_push_tunnel_right(session, height, TUNNEL_6);
    }
    else
    {
        imageId = SPR_CHAIRLIFT_CABLE_FLAT_SW_NE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());
        paint_util_push_tunnel_left(session, height, TUNNEL_6);
    }

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 32, 0x20);
}

/** rct2: 0x00743FD8 */
static void chairlift_paint_25_deg_up(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                      sint32 height, rct_tile_element * tileElement)
{
    uint32 imageId;

    switch (direction)
    {
    case 0:
        imageId = SPR_CHAIRLIFT_CABLE_UP_SW_NE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());
        paint_util_push_tunnel_left(session, height - 8, TUNNEL_7);
        break;

    case 1:
        imageId = SPR_CHAIRLIFT_CABLE_UP_NW_SE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());
        paint_util_push_tunnel_right(session, height + 8, TUNNEL_8);
        break;

    case 2:
        imageId = SPR_CHAIRLIFT_CABLE_UP_NE_SW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());
        paint_util_push_tunnel_left(session, height + 8, TUNNEL_8);
        break;

    case 3:
        imageId = SPR_CHAIRLIFT_CABLE_UP_SE_NW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());
        paint_util_push_tunnel_right(session, height - 8, TUNNEL_7);
        break;
    }

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 56, 0x20);
}

/** rct2: 0x00743FD8 */
static void chairlift_paint_flat_to_25_deg_up(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                              sint32 height, rct_tile_element * tileElement)
{
    uint32 imageId;

    switch (direction)
    {
    case 0:
        imageId = SPR_20508 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());

        imageId = SPR_20520 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_left(session, height, TUNNEL_6);
        break;

    case 1:
        imageId = SPR_20509 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());

        imageId = SPR_20521 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_right(session, height, TUNNEL_8);
        break;

    case 2:
        imageId = SPR_20510 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());

        imageId = SPR_20522 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_left(session, height, TUNNEL_8);
        break;

    case 3:
        imageId = SPR_20511 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());

        imageId = SPR_20523 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_right(session, height, TUNNEL_6);
        break;
    }

    chairlift_paint_util_draw_supports(session, SEGMENT_C4, height);
    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 48, 0x20);
}

/** rct2: 0x00743FF8 */
static void chairlift_paint_25_deg_up_to_flat(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                              sint32 height, rct_tile_element * tileElement)
{
    uint32 imageId;

    switch (direction)
    {
    case 0:
        imageId = SPR_20512 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());

        imageId = SPR_20524 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_left(session, height - 8, TUNNEL_6);
        break;

    case 1:
        imageId = SPR_20513 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());

        imageId = SPR_20525 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_right(session, height + 8, TUNNEL_14);
        break;

    case 2:
        imageId = SPR_20514 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 32, 6, 2, height, 0, 13, height + 28, get_current_rotation());

        imageId = SPR_20526 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_left(session, height + 8, TUNNEL_14);
        break;

    case 3:
        imageId = SPR_20515 | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 6, 32, 2, height, 13, 0, height + 28, get_current_rotation());

        imageId = SPR_20527 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 4, 4, 25, height, 14, 14, height + 1, get_current_rotation());

        paint_util_push_tunnel_right(session, height - 8, TUNNEL_6);
        break;
    }

    chairlift_paint_util_draw_supports(session, SEGMENT_C4, height);
    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 40, 0x20);
}

/** rct2: 0x00744008 */
static void chairlift_paint_25_deg_down(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                        sint32 height, rct_tile_element * tileElement)
{
    chairlift_paint_25_deg_up(session, rideIndex, trackSequence, (direction + 2) % 4, height, tileElement);
}

/** rct2: 0x00744018 */
static void chairlift_paint_flat_to_25_deg_down(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                                sint32 height, rct_tile_element * tileElement)
{
    chairlift_paint_25_deg_up_to_flat(session, rideIndex, trackSequence, (direction + 2) % 4, height, tileElement);
}

/** rct2: 0x00744028 */
static void chairlift_paint_25_deg_down_to_flat(paint_session * session, uint8 rideIndex, uint8 trackSequence, uint8 direction,
                                                sint32 height, rct_tile_element * tileElement)
{
    chairlift_paint_flat_to_25_deg_up(session, rideIndex, trackSequence, (direction + 2) % 4, height, tileElement);
}

/** rct2: 0x00744038 */
static void chairlift_paint_left_quarter_turn_1_tile(paint_session * session, uint8 rideIndex, uint8 trackSequence,
                                                     uint8 direction, sint32 height, rct_tile_element * tileElement)
{
    uint32 imageId;

    switch (direction)
    {
    case 0:
        imageId = SPR_CHAIRLIFT_CORNER_NW_SW | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 16, 16, 2, height, 16, 0, height + 28, get_current_rotation());

        imageId = SPR_20532 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 16, 4, height, get_current_rotation());

        imageId = SPR_20536 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 28, 4, height, get_current_rotation());

        paint_util_push_tunnel_left(session, height, TUNNEL_6);
        break;

    case 1:
        imageId = SPR_CHAIRLIFT_CORNER_NW_NE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 16, 16, 2, height, 0, 0, height + 28, get_current_rotation());

        imageId = SPR_20533 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 16, 4, height, get_current_rotation());

        imageId = SPR_20537 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 4, 16, height, get_current_rotation());
        break;

    case 2:
        imageId = SPR_CHAIRLIFT_CORNER_SE_NE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 16, 16, 2, height, 0, 16, height + 28, get_current_rotation());

        imageId = SPR_20534 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 4, 16, height, get_current_rotation());

        imageId = SPR_20538 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 16, 28, height, get_current_rotation());

        paint_util_push_tunnel_right(session, height, TUNNEL_6);
        break;

    case 3:
        imageId = SPR_CHAIRLIFT_CORNER_SW_SE | session->TrackColours[SCHEME_TRACK];
        sub_98197C(session, imageId, 0, 0, 16, 16, 2, height, 16, 16, height + 28, get_current_rotation());

        imageId = SPR_20535 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 28, 16, height, get_current_rotation());

        imageId = SPR_20539 | session->TrackColours[SCHEME_SUPPORTS];
        sub_98197C(session, imageId, 0, 0, 2, 2, 27, height, 16, 28, height, get_current_rotation());

        paint_util_push_tunnel_left(session, height, TUNNEL_6);
        paint_util_push_tunnel_right(session, height, TUNNEL_6);
        break;
    }

    chairlift_paint_util_draw_supports(session, paint_util_rotate_segments(SEGMENT_C8 | SEGMENT_D0, direction), height);

    paint_util_set_segment_support_height(session, SEGMENTS_ALL, 0xFFFF, 0);
    paint_util_set_general_support_height(session, height + 32, 0x20);
}

/** rct2: 0x00744048 */
static void chairlift_paint_right_quarter_turn_1_tile(paint_session * session, uint8 rideIndex, uint8 trackSequence,
                                                      uint8 direction, sint32 height, rct_tile_element * tileElement)
{
    chairlift_paint_left_quarter_turn_1_tile(session, rideIndex, trackSequence, (direction + 3) % 4, height, tileElement);
}

/* 0x008AAA0C */
TRACK_PAINT_FUNCTION get_track_paint_function_chairlift(sint32 trackType, sint32 direction)
{
    switch (trackType)
    {
    case TRACK_ELEM_BEGIN_STATION:
    case TRACK_ELEM_MIDDLE_STATION:
    case TRACK_ELEM_END_STATION:
        return chairlift_paint_station;

    case TRACK_ELEM_FLAT:
        return chairlift_paint_flat;

    case TRACK_ELEM_FLAT_TO_25_DEG_UP:
        return chairlift_paint_flat_to_25_deg_up;
    case TRACK_ELEM_25_DEG_UP:
        return chairlift_paint_25_deg_up;
    case TRACK_ELEM_25_DEG_UP_TO_FLAT:
        return chairlift_paint_25_deg_up_to_flat;

    case TRACK_ELEM_FLAT_TO_25_DEG_DOWN:
        return chairlift_paint_flat_to_25_deg_down;
    case TRACK_ELEM_25_DEG_DOWN:
        return chairlift_paint_25_deg_down;
    case TRACK_ELEM_25_DEG_DOWN_TO_FLAT:
        return chairlift_paint_25_deg_down_to_flat;

    case TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE:
        return chairlift_paint_left_quarter_turn_1_tile;
    case TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE:
        return chairlift_paint_right_quarter_turn_1_tile;
    }

    return NULL;
}
