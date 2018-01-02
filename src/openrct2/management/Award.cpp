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

#include "../config/Config.h"
#include "../core/Util.hpp"
#include "../interface/window.h"
#include "../localisation/string_ids.h"
#include "../peep/Peep.h"
#include "../ride/Ride.h"
#include "../scenario/scenario.h"
#include "Award.h"
#include "NewsItem.h"

#define NEGATIVE 0
#define POSITIVE 1

static const uint8 AwardPositiveMap[] =
{
    NEGATIVE, // PARK_AWARD_MOST_UNTIDY
    POSITIVE, // PARK_AWARD_MOST_TIDY
    POSITIVE, // PARK_AWARD_BEST_ROLLERCOASTERS
    POSITIVE, // PARK_AWARD_BEST_VALUE
    POSITIVE, // PARK_AWARD_MOST_BEAUTIFUL
    NEGATIVE, // PARK_AWARD_WORST_VALUE
    POSITIVE, // PARK_AWARD_SAFEST
    POSITIVE, // PARK_AWARD_BEST_STAFF
    POSITIVE, // PARK_AWARD_BEST_FOOD
    NEGATIVE, // PARK_AWARD_WORST_FOOD
    POSITIVE, // PARK_AWARD_BEST_RESTROOMS
    NEGATIVE, // PARK_AWARD_MOST_DISAPPOINTING
    POSITIVE, // PARK_AWARD_BEST_WATER_RIDES
    POSITIVE, // PARK_AWARD_BEST_CUSTOM_DESIGNED_RIDES
    POSITIVE, // PARK_AWARD_MOST_DAZZLING_RIDE_COLOURS
    NEGATIVE, // PARK_AWARD_MOST_CONFUSING_LAYOUT
    POSITIVE, // PARK_AWARD_BEST_GENTLE_RIDES
};

static const rct_string_id AwardNewsStrings[] =
{
    STR_NEWS_ITEM_AWARD_MOST_UNTIDY,
    STR_NEWS_ITEM_MOST_TIDY,
    STR_NEWS_ITEM_BEST_ROLLERCOASTERS,
    STR_NEWS_ITEM_BEST_VALUE,
    STR_NEWS_ITEM_MOST_BEAUTIFUL,
    STR_NEWS_ITEM_WORST_VALUE,
    STR_NEWS_ITEM_SAFEST,
    STR_NEWS_ITEM_BEST_STAFF,
    STR_NEWS_ITEM_BEST_FOOD,
    STR_NEWS_ITEM_WORST_FOOD,
    STR_NEWS_ITEM_BEST_RESTROOMS,
    STR_NEWS_ITEM_MOST_DISAPPOINTING,
    STR_NEWS_ITEM_BEST_WATER_RIDES,
    STR_NEWS_ITEM_BEST_CUSTOM_DESIGNED_RIDES,
    STR_NEWS_ITEM_MOST_DAZZLING_RIDE_COLOURS,
    STR_NEWS_ITEM_MOST_CONFUSING_LAYOUT,
    STR_NEWS_ITEM_BEST_GENTLE_RIDES,
};

Award gCurrentAwards[MAX_AWARDS];

bool award_is_positive(sint32 type)
{
    return AwardPositiveMap[type];
}

#pragma region Award checks

/** More than 1/16 of the total guests must be thinking untidy thoughts. */
static bool award_is_deserved_most_untidy(sint32 awardType, sint32 activeAwardTypes)
{
    uint16 spriteIndex;
    rct_peep * peep;
    sint32 negativeCount;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_BEAUTIFUL))
        return false;
    if (activeAwardTypes & (1 << PARK_AWARD_BEST_STAFF))
        return false;
    if (activeAwardTypes & (1 << PARK_AWARD_MOST_TIDY))
        return false;

    negativeCount = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        if (peep->thoughts[0].var_2 > 5)
            continue;

        if (peep->thoughts[0].type == PEEP_THOUGHT_TYPE_BAD_LITTER ||
            peep->thoughts[0].type == PEEP_THOUGHT_TYPE_PATH_DISGUSTING ||
            peep->thoughts[0].type == PEEP_THOUGHT_TYPE_VANDALISM)
        {
            negativeCount++;
        }
    }

    return (negativeCount > gNumGuestsInPark / 16);
}

/** More than 1/64 of the total guests must be thinking tidy thoughts and less than 6 guests thinking untidy thoughts. */
static bool award_is_deserved_most_tidy(sint32 awardType, sint32 activeAwardTypes)
{
    uint16 spriteIndex;
    rct_peep * peep;
    sint32 positiveCount;
    sint32 negativeCount;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_UNTIDY))
        return false;
    if (activeAwardTypes & (1 << PARK_AWARD_MOST_DISAPPOINTING))
        return false;

    positiveCount = 0;
    negativeCount = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        if (peep->thoughts[0].var_2 > 5)
            continue;

        if (peep->thoughts[0].type == PEEP_THOUGHT_TYPE_VERY_CLEAN)
            positiveCount++;

        if (peep->thoughts[0].type == PEEP_THOUGHT_TYPE_BAD_LITTER ||
            peep->thoughts[0].type == PEEP_THOUGHT_TYPE_PATH_DISGUSTING ||
            peep->thoughts[0].type == PEEP_THOUGHT_TYPE_VANDALISM
            )
        {
            negativeCount++;
        }
    }

    return (negativeCount <= 5 && positiveCount > gNumGuestsInPark / 64);
}

/** At least 6 open roller coasters. */
static bool award_is_deserved_best_rollercoasters(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, rollerCoasters;
    Ride           * ride;
    rct_ride_entry * rideEntry;

    rollerCoasters = 0;
    FOR_ALL_RIDES(i, ride)
    {
        rideEntry = get_ride_entry(ride->subtype);
        if (rideEntry == nullptr)
        {
            continue;
        }

        if (ride->status != RIDE_STATUS_OPEN || (ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED))
        {
            continue;
        }

        if (!ride_entry_has_category(rideEntry, RIDE_CATEGORY_ROLLERCOASTER))
        {
            continue;
        }

        rollerCoasters++;
    }

    return (rollerCoasters >= 6);
}

/** Entrance fee is 0.10 less than half of the total ride value. */
static bool award_is_deserved_best_value(sint32 awardType, sint32 activeAwardTypes)
{
    if (activeAwardTypes & (1 << PARK_AWARD_WORST_VALUE))
        return false;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_DISAPPOINTING))
        return false;

    if (gParkFlags & (PARK_FLAGS_NO_MONEY | PARK_FLAGS_PARK_FREE_ENTRY))
        return false;

    if (gTotalRideValueForMoney < MONEY(10, 00))
        return false;

    if (park_get_entrance_fee() + MONEY(0, 10) >= gTotalRideValueForMoney / 2)
        return false;

    return true;
}

/** More than 1/128 of the total guests must be thinking scenic thoughts and fewer than 16 untidy thoughts. */
static bool award_is_deserved_most_beautiful(sint32 awardType, sint32 activeAwardTypes)
{
    uint16 spriteIndex;
    rct_peep * peep;
    sint32 positiveCount;
    sint32 negativeCount;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_UNTIDY))
        return false;
    if (activeAwardTypes & (1 << PARK_AWARD_MOST_DISAPPOINTING))
        return false;

    positiveCount = 0;
    negativeCount = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        if (peep->thoughts[0].var_2 > 5)
            continue;

        if (peep->thoughts[0].type == PEEP_THOUGHT_TYPE_SCENERY)
            positiveCount++;

        if (peep->thoughts[0].type == PEEP_THOUGHT_TYPE_BAD_LITTER ||
            peep->thoughts[0].type == PEEP_THOUGHT_TYPE_PATH_DISGUSTING ||
            peep->thoughts[0].type == PEEP_THOUGHT_TYPE_VANDALISM)
        {
            negativeCount++;
        }
    }

    return (negativeCount <= 15 && positiveCount > gNumGuestsInPark / 128);
}

/** Entrance fee is more than total ride value. */
static bool award_is_deserved_worst_value(sint32 awardType, sint32 activeAwardTypes)
{
    if (activeAwardTypes & (1 << PARK_AWARD_BEST_VALUE))
        return false;
    if (gParkFlags & PARK_FLAGS_NO_MONEY)
        return false;

    money32 parkEntranceFee = park_get_entrance_fee();
    if (parkEntranceFee == MONEY(0, 00))
        return false;
    if (parkEntranceFee <= gTotalRideValueForMoney)
        return false;
    return true;
}

/** No more than 2 people who think the vandalism is bad and no crashes. */
static bool award_is_deserved_safest(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, peepsWhoDislikeVandalism;
    uint16 spriteIndex;
    rct_peep * peep;
    Ride     * ride;

    peepsWhoDislikeVandalism = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;
        if (peep->thoughts[0].var_2 <= 5 && peep->thoughts[0].type == PEEP_THOUGHT_TYPE_VANDALISM)
            peepsWhoDislikeVandalism++;
    }

    if (peepsWhoDislikeVandalism > 2)
        return false;

    // Check for rides that have crashed maybe?
    FOR_ALL_RIDES(i, ride)
    {
        if (ride->last_crash_type != RIDE_CRASH_TYPE_NONE)
            return false;
    }

    return true;
}

/** All staff types, at least 20 staff, one staff per 32 peeps. */
static bool award_is_deserved_best_staff(sint32 awardType, sint32 activeAwardTypes)
{
    uint16 spriteIndex;
    rct_peep * peep;
    sint32 peepCount, staffCount;
    sint32 staffTypeFlags;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_UNTIDY))
        return false;

    peepCount      = 0;
    staffCount     = 0;
    staffTypeFlags = 0;
    FOR_ALL_PEEPS(spriteIndex, peep)
    {
        if (peep->type == PEEP_TYPE_STAFF)
        {
            staffCount++;
            staffTypeFlags |= (1 << peep->staff_type);
        }
        else
        {
            peepCount++;
        }
    }

    return ((staffTypeFlags & 0xF) && staffCount >= 20 && staffCount >= peepCount / 32);

}

/** At least 7 shops, 4 unique, one shop per 128 guests and no more than 12 hungry guests. */
static bool award_is_deserved_best_food(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, hungryPeeps, shops, uniqueShops;
    uint64 shopTypes;
    Ride           * ride;
    rct_ride_entry * rideEntry;
    uint16 spriteIndex;
    rct_peep * peep;

    if (activeAwardTypes & (1 << PARK_AWARD_WORST_FOOD))
        return false;

    shops       = 0;
    uniqueShops = 0;
    shopTypes   = 0;
    FOR_ALL_RIDES(i, ride)
    {
        if (ride->status != RIDE_STATUS_OPEN)
            continue;
        if (!ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD))
            continue;

        shops++;
        rideEntry = get_ride_entry(ride->subtype);
        if (rideEntry == nullptr)
        {
            continue;
        }
        if (!(shopTypes & (1ULL << rideEntry->shop_item)))
        {
            shopTypes |= (1ULL << rideEntry->shop_item);
            uniqueShops++;
        }
    }

    if (shops < 7 || uniqueShops < 4 || shops < gNumGuestsInPark / 128)
        return false;

    // Count hungry peeps
    hungryPeeps = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        if (peep->thoughts[0].var_2 <= 5 && peep->thoughts[0].type == PEEP_THOUGHT_TYPE_HUNGRY)
            hungryPeeps++;
    }

    return (hungryPeeps <= 12);
}

/** No more than 2 unique shops, less than one shop per 256 guests and more than 15 hungry guests. */
static bool award_is_deserved_worst_food(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, hungryPeeps, shops, uniqueShops;
    uint64 shopTypes;
    Ride           * ride;
    rct_ride_entry * rideEntry;
    uint16 spriteIndex;
    rct_peep * peep;

    if (activeAwardTypes & (1 << PARK_AWARD_BEST_FOOD))
        return false;

    shops       = 0;
    uniqueShops = 0;
    shopTypes   = 0;
    FOR_ALL_RIDES(i, ride)
    {
        if (ride->status != RIDE_STATUS_OPEN)
            continue;
        if (!ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_SELLS_FOOD))
            continue;

        shops++;
        rideEntry = get_ride_entry(ride->subtype);
        if (rideEntry == nullptr)
        {
            continue;
        }
        if (!(shopTypes & (1ULL << rideEntry->shop_item)))
        {
            shopTypes |= (1ULL << rideEntry->shop_item);
            uniqueShops++;
        }
    }

    if (uniqueShops > 2 || shops > gNumGuestsInPark / 256)
        return false;

    // Count hungry peeps
    hungryPeeps = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        if (peep->thoughts[0].var_2 <= 5 && peep->thoughts[0].type == PEEP_THOUGHT_TYPE_HUNGRY)
            hungryPeeps++;
    }

    return (hungryPeeps > 15);
}

/** At least 4 restrooms, 1 restroom per 128 guests and no more than 16 guests who think they need the restroom. */
static bool award_is_deserved_best_restrooms(sint32 awardType, sint32 activeAwardTypes)
{
    uint32 i, numRestrooms, guestsWhoNeedRestroom;
    Ride * ride;
    uint16 spriteIndex;
    rct_peep * peep;

    // Count open restrooms
    numRestrooms = 0;
    FOR_ALL_RIDES(i, ride)
    {
        if (ride->type == RIDE_TYPE_TOILETS && ride->status == RIDE_STATUS_OPEN)
            numRestrooms++;
    }

    // At least 4 open restrooms
    if (numRestrooms < 4)
        return false;

    // At least one open restroom for every 128 guests
    if (numRestrooms < gNumGuestsInPark / 128U)
        return false;

    // Count number of guests who are thinking they need the restroom
    guestsWhoNeedRestroom = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        if (peep->thoughts[0].var_2 <= 5 && peep->thoughts[0].type == PEEP_THOUGHT_TYPE_BATHROOM)
            guestsWhoNeedRestroom++;
    }

    return (guestsWhoNeedRestroom <= 16);
}

/** More than half of the rides have satisfaction <= 6 and park rating <= 650. */
static bool award_is_deserved_most_disappointing(sint32 awardType, sint32 activeAwardTypes)
{
    uint32 i, countedRides, disappointingRides;
    Ride * ride;

    if (activeAwardTypes & (1 << PARK_AWARD_BEST_VALUE))
        return false;
    if (gParkRating > 650)
        return false;

    // Count the number of disappointing rides
    countedRides       = 0;
    disappointingRides = 0;

    FOR_ALL_RIDES(i, ride)
    {
        if (ride->excitement == RIDE_RATING_UNDEFINED || ride->popularity == 0xFF)
            continue;

        countedRides++;

        // Unpopular
        if (ride->popularity <= 6)
            disappointingRides++;
    }

    // Half of the rides are disappointing
    return (disappointingRides >= countedRides / 2);
}

/** At least 6 open water rides. */
static bool award_is_deserved_best_water_rides(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, waterRides;
    Ride           * ride;
    rct_ride_entry * rideEntry;

    waterRides = 0;
    FOR_ALL_RIDES(i, ride)
    {
        rideEntry = get_ride_entry(ride->subtype);
        if (rideEntry == nullptr)
        {
            continue;
        }

        if (ride->status != RIDE_STATUS_OPEN || (ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED))
        {
            continue;
        }

        if (!ride_entry_has_category(rideEntry, RIDE_CATEGORY_WATER))
        {
            continue;
        }

        waterRides++;
    }

    return (waterRides >= 6);
}

/** At least 6 custom designed rides. */
static bool award_is_deserved_best_custom_designed_rides(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, customDesignedRides;
    Ride * ride;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_DISAPPOINTING))
        return false;

    customDesignedRides = 0;
    FOR_ALL_RIDES(i, ride)
    {
        if (!ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_HAS_TRACK))
            continue;
        if (ride->lifecycle_flags & RIDE_LIFECYCLE_NOT_CUSTOM_DESIGN)
            continue;
        if (ride->excitement < RIDE_RATING(5, 50))
            continue;
        if (ride->status != RIDE_STATUS_OPEN || (ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED))
            continue;

        customDesignedRides++;
    }

    return (customDesignedRides >= 6);
}

/** At least 5 colourful rides and more than half of the rides are colourful. */
static const uint8 dazzling_ride_colours[] = {5, 14, 20, 30};

static bool award_is_deserved_most_dazzling_ride_colours(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, countedRides, colourfulRides;
    Ride * ride;
    uint8 mainTrackColour;

    if (activeAwardTypes & (1 << PARK_AWARD_MOST_DISAPPOINTING))
        return false;

    countedRides   = 0;
    colourfulRides = 0;
    FOR_ALL_RIDES(i, ride)
    {
        if (!ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_HAS_TRACK))
            continue;

        countedRides++;

        mainTrackColour = ride->track_colour_main[0];
        for (auto dazzling_ride_colour : dazzling_ride_colours)
        {
            if (mainTrackColour == dazzling_ride_colour)
            {
                colourfulRides++;
                break;
            }
        }
    }

    return (colourfulRides >= 5 && colourfulRides >= countedRides - colourfulRides);
}

/** At least 10 peeps and more than 1/64 of total guests are lost or can't find something. */
static bool award_is_deserved_most_confusing_layout(sint32 awardType, sint32 activeAwardTypes)
{
    uint32 peepsCounted, peepsLost;
    uint16 spriteIndex;
    rct_peep * peep;

    peepsCounted = 0;
    peepsLost    = 0;
    FOR_ALL_GUESTS(spriteIndex, peep)
    {
        if (peep->outside_of_park != 0)
            continue;

        peepsCounted++;
        if (peep->thoughts[0].var_2 <= 5 && (peep->thoughts[0].type == PEEP_THOUGHT_TYPE_LOST || peep->thoughts[0].type == PEEP_THOUGHT_TYPE_CANT_FIND))
            peepsLost++;
    }

    return (peepsLost >= 10 && peepsLost >= peepsCounted / 64);
}

/** At least 10 open gentle rides. */
static bool award_is_deserved_best_gentle_rides(sint32 awardType, sint32 activeAwardTypes)
{
    sint32 i, gentleRides;
    Ride           * ride;
    rct_ride_entry * rideEntry;

    gentleRides = 0;
    FOR_ALL_RIDES(i, ride)
    {
        rideEntry = get_ride_entry(ride->subtype);
        if (rideEntry == nullptr)
        {
            continue;
        }

        if (ride->status != RIDE_STATUS_OPEN || (ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED))
        {
            continue;
        }

        if (!ride_entry_has_category(rideEntry, RIDE_CATEGORY_GENTLE))
        {
            continue;
        }

        gentleRides++;
    }

    return (gentleRides >= 10);
}

typedef bool (* award_deserved_check)(sint32, sint32);

static const award_deserved_check _awardChecks[] =
{
    award_is_deserved_most_untidy,
    award_is_deserved_most_tidy,
    award_is_deserved_best_rollercoasters,
    award_is_deserved_best_value,
    award_is_deserved_most_beautiful,
    award_is_deserved_worst_value,
    award_is_deserved_safest,
    award_is_deserved_best_staff,
    award_is_deserved_best_food,
    award_is_deserved_worst_food,
    award_is_deserved_best_restrooms,
    award_is_deserved_most_disappointing,
    award_is_deserved_best_water_rides,
    award_is_deserved_best_custom_designed_rides,
    award_is_deserved_most_dazzling_ride_colours,
    award_is_deserved_most_confusing_layout,
    award_is_deserved_best_gentle_rides
};

static bool award_is_deserved(sint32 awardType, sint32 activeAwardTypes)
{
    return _awardChecks[awardType](awardType, activeAwardTypes);
}

#pragma endregion

void award_reset()
{
    for (auto &award : gCurrentAwards)
    {
        award.Time = 0;
        award.Type = 0;
    }
}

/**
 *
 *  rct2: 0x0066A86C
 */
void award_update_all()
{
    // Only add new awards if park is open
    if (gParkFlags & PARK_FLAGS_PARK_OPEN)
    {
        // Set active award types as flags
        sint32      activeAwardTypes    = 0;
        sint32      freeAwardEntryIndex = -1;
        for (sint32 i                   = 0; i < MAX_AWARDS; i++)
        {
            if (gCurrentAwards[i].Time != 0)
                activeAwardTypes |= (1 << gCurrentAwards[i].Type);
            else if (freeAwardEntryIndex == -1)
                freeAwardEntryIndex = i;
        }

        // Check if there was a free award entry
        if (freeAwardEntryIndex != -1)
        {
            // Get a random award type not already active
            sint32 awardType;
            do
            {
                awardType = (((scenario_rand() & 0xFF) * 17) >> 8) & 0xFF;
            }
            while (activeAwardTypes & (1 << awardType));

            // Check if award is deserved
            if (award_is_deserved(awardType, activeAwardTypes))
            {
                // Add award
                gCurrentAwards[freeAwardEntryIndex].Type = awardType;
                gCurrentAwards[freeAwardEntryIndex].Time = 5;
                if (gConfigNotifications.park_award)
                {
                    news_item_add_to_queue(NEWS_ITEM_AWARD, AwardNewsStrings[awardType], 0);
                }
                window_invalidate_by_class(WC_PARK_INFORMATION);
            }
        }
    }

    // Decrease award times
    for (auto &award : gCurrentAwards)
    {
        if (award.Time != 0)
            if (--award.Time == 0)
                window_invalidate_by_class(WC_PARK_INFORMATION);
    }
}
