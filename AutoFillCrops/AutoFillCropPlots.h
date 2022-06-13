#pragma once
#include "API/Ark/Ark.h"

// Default configuration values.
#define FILLCROPS_DEFAULT_RADIUS 10800.0F
#define FILLCROPS_DEFAULT_CHAT_TRIGGER "/fill"
#define FILLCROPS_DEFAULT_SEARCH_IN_PLAYER true
#define FILLCROPS_DEFAULT_SEARCH_IN_DUNGBEETLES true

// Min radius, arbitrarily set to 1.
#define FILLCROPS_RADIUS_MIN 1
// Max radius arbitrarily set to 100000 but it is not recommended to use such a high value (that is a radius of 333 foundations in length, diameter of 667 foundations).
#define FILLCROPS_RADIUS_MAX 100000
// 5 seconds delay between command usage.
#define FILLCROPS_INTERVAL_DELAY 5i64

#define FILLCROPS_CONFIG_RADIUS                 "actionRadius="
#define FILLCROPS_CONFIG_CHAT_TRIGGER           "chatTrigger="
#define FILLCROPS_CONFIG_SEARCH_IN_PLAYER       "searchInPlayerInventory="
#define FILLCROPS_CONFIG_SEARCH_IN_DUNGBEETLES  "searchInDungBeetlesInventories="

#define FILLCROPS_FERTILIZER_COMPOST_BP         L"Blueprint'/Game/PrimalEarth/CoreBlueprints/Items/Consumables/PrimalItemConsumable_Fertilizer_Compost.PrimalItemConsumable_Fertilizer_Compost'"
#define FILLCROPS_DUNG_BEETLE_BP                L"Blueprint'/Game/PrimalEarth/Dinos/DungBeetle/DungBeetle_Character_BP.DungBeetle_Character_BP'"
#define FILLCROPS_ABERRANT_DUNG_BEETLE_BP       L"Blueprint'/Game/PrimalEarth/Dinos/DungBeetle/DungBeetle_Character_BP_Aberrant.DungBeetle_Character_BP_Aberrant'"

struct AutoFillCropPlotsConfig
{
	float Radius;
	std::string ChatTriger;
	bool SearchInPlayerInventory;
	bool SearchInDungBeetlesInventories;
};
