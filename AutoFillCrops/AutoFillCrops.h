#pragma once
#include "API/Ark/Ark.h"

UClass* MyBPLoadClass(FString* blueprintPath);
std::pair<float, std::string> GetFillCropsConfig();
