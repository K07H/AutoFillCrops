#include <filesystem>
#include "AutoFillCropPlots.h"
#include "ReadFile.h"

#pragma comment(lib, "ArkApi.lib")

UClass* MyBPLoadClass(const wchar_t* blueprintPath)
{
	static std::map<FString, UClass*> loadedClasses;

	FString bp = FString(blueprintPath);
	if (loadedClasses.size() > 0)
		for (auto it = loadedClasses.begin(), end = loadedClasses.end(); it != end; it++)
			if ((*it).first.Compare(bp) == 0)
				return (*it).second;
	UClass* toAdd = UVictoryCore::BPLoadClass(&bp);
	if (toAdd != nullptr)
		loadedClasses[bp] = toAdd;
	return toAdd;
}

FString LoadConfigFileContent()
{
	FString fileContent = L"";
	std::string configPath = "./ArkApi/Plugins/AutoFillCrops/Config.txt";
	try
	{
		if (std::filesystem::exists(configPath))
		{
			std::string content = ReadFile(configPath);
			if (content.empty() || content.compare("\r\n") == 0 || content.compare("\n") == 0) // If config file is empty.
				return fileContent;
			fileContent = FString(content);
			fileContent.ReplaceInline(L"\r\n", L"\n"); // Replace all Windows newlines so that we just have to give Unix newline delimiter to std::getline.
		}
		else
			Log::GetLog()->error("Failed to load AutoFillCrops configuration file. File not found.");
	}
	catch (const std::exception&)
	{
		fileContent = L"";
		Log::GetLog()->error("Failed to load AutoFillCrops configuration file. Please verify its content.");
	}
	return fileContent;
}

AutoFillCropPlotsConfig GetFillCropsRadiusConfig()
{
	// Initialize config to default values.
	AutoFillCropPlotsConfig config = AutoFillCropPlotsConfig();
	config.Radius = FILLCROPS_DEFAULT_RADIUS;
	config.ChatTriger = FILLCROPS_DEFAULT_CHAT_TRIGGER;
	config.SearchInPlayerInventory = FILLCROPS_DEFAULT_SEARCH_IN_PLAYER;
	config.SearchInDungBeetlesInventories = FILLCROPS_DEFAULT_SEARCH_IN_DUNGBEETLES;

	// Read config file content.
	FString fileContent = LoadConfigFileContent();
	if (!fileContent.IsEmpty())
	{
		std::string item;
		std::stringstream ss(fileContent.ToString());

		// Grab config prefixes lengths.
		int actionRadiusStrlen = (int)std::string(FILLCROPS_CONFIG_RADIUS).size();
		int chatTriggerStrlen = (int)std::string(FILLCROPS_CONFIG_CHAT_TRIGGER).size();
		int searchInPlayerStrlen = (int)std::string(FILLCROPS_CONFIG_SEARCH_IN_PLAYER).size();
		int searchInDungBeetlesStrlen = (int)std::string(FILLCROPS_CONFIG_SEARCH_IN_DUNGBEETLES).size();

		// Try to extract any valid config line from the config file.
		bool radiusNotFound = true;
		while (std::getline(ss, item, '\n'))
			if (!item.empty() && item.size() > 0)
			{
				try
				{
					if (item.rfind(FILLCROPS_CONFIG_RADIUS, 0) == 0)
					{
						config.Radius = FILLCROPS_DEFAULT_RADIUS;
						if (item.size() > actionRadiusStrlen)
						{
							int radiusInt = std::atoi(item.substr(actionRadiusStrlen).c_str());
							if (radiusInt >= FILLCROPS_RADIUS_MIN && radiusInt <= FILLCROPS_RADIUS_MAX)
								config.Radius = (float)radiusInt;
						}
					}
					else if (item.rfind(FILLCROPS_CONFIG_CHAT_TRIGGER, 0) == 0)
						config.ChatTriger = (item.size() > chatTriggerStrlen ? item.substr(chatTriggerStrlen) : FILLCROPS_DEFAULT_CHAT_TRIGGER);
					else if (item.rfind(FILLCROPS_CONFIG_SEARCH_IN_PLAYER, 0) == 0)
					{
						if (item.size() > searchInPlayerStrlen)
						{
							FString val = FString(item.substr(searchInPlayerStrlen)).TrimStartAndEnd();
							config.SearchInPlayerInventory = (val.Compare(L"true", ESearchCase::IgnoreCase) == 0);
						}
						else
							config.SearchInPlayerInventory = FILLCROPS_DEFAULT_SEARCH_IN_PLAYER;
					}
					else if (item.rfind(FILLCROPS_CONFIG_SEARCH_IN_DUNGBEETLES, 0) == 0)
					{
						if (item.size() > searchInDungBeetlesStrlen)
						{
							FString val = FString(item.substr(searchInDungBeetlesStrlen)).TrimStartAndEnd();
							config.SearchInDungBeetlesInventories = (val.Compare(L"true", ESearchCase::IgnoreCase) == 0);
						}
						else
							config.SearchInDungBeetlesInventories = FILLCROPS_DEFAULT_SEARCH_IN_DUNGBEETLES;
					}
				}
				catch (const std::exception&) {}
			}
	}
	return config;
}

const AutoFillCropPlotsConfig& GetFillCropsConfig()
{
	static bool _notInitialized = true;
	static AutoFillCropPlotsConfig _config = AutoFillCropPlotsConfig();

	if (_notInitialized)
	{
		_notInitialized = false;
		AutoFillCropPlotsConfig conf = GetFillCropsRadiusConfig();
		_config.Radius = conf.Radius;
		_config.ChatTriger = conf.ChatTriger;
		_config.SearchInPlayerInventory = conf.SearchInPlayerInventory;
		_config.SearchInDungBeetlesInventories = conf.SearchInDungBeetlesInventories;
		Log::GetLog()->info("Parsed configuration file. Radius=[" + std::to_string(_config.Radius) + "] ChatCmd=[" + _config.ChatTriger + "] SearchInPlayer[" + std::to_string(_config.SearchInPlayerInventory) + "] SearchInDungBeetles[" + std::to_string(_config.SearchInDungBeetlesInventories) + "].");
	}
	return _config;
}

void AddToFertilizersList(TArray<UPrimalItem*>* fertilizers, UPrimalItem* fert)
{
	if (fertilizers != nullptr && fert != nullptr)
	{
		bool found = false;
		int i = 0;
		int len = fertilizers->Num();
		FItemNetID fertId = fert->ItemIDField();

		while (!found && i < len)
		{
			UPrimalItem* current = (*fertilizers)[i];
			if (current != nullptr)
			{
				FItemNetID itemId = current->ItemIDField();
				if (itemId.ItemID1 == fertId.ItemID1 && itemId.ItemID2 == fertId.ItemID2)
					found = true;
			}
			i++;
		}

		if (!found)
			fertilizers->Add(fert);
	}
}

void AddFertilizerToInventory(UPrimalInventoryComponent* inventory, int amount)
{
	UClass* bpUClass = MyBPLoadClass(FILLCROPS_FERTILIZER_COMPOST_BP);
	if (bpUClass != nullptr)
		for (int i = 0; i < amount; i++)
		{
			UPrimalItem* item = UPrimalItem::AddNewItem(TSubclassOf<UPrimalItem>(bpUClass), nullptr, false, true, 1.0F, false, 1, false, 1.0F, false, nullptr, 1.0F);
			if (item != nullptr)
				inventory->AddItemObject(item);
		}
}

bool ProcessFillCropsCommand(AShooterPlayerController* PC, const AutoFillCropPlotsConfig& config) // float radius, const std::string& chatCmdTrigger, bool searchInPlayerInventory, bool SearchInDungBeetlesInventories)
{
	UWorld* world = ArkApi::GetApiUtils().GetWorld();
	if (world == nullptr) // If world not found.
		return true; // Prevent normal chat execution by returning true.

	// Get fertilizer template.
	auto t = L"";
	UClass* fertilizerClassObj = MyBPLoadClass(FILLCROPS_FERTILIZER_COMPOST_BP);
	if (fertilizerClassObj == nullptr)
	{
		Log::GetLog()->error("Failed to load fertilizer class for " + config.ChatTriger + " command.");
		return true; // Prevent normal chat execution by returning true.
	}
	TSubclassOf<UPrimalItem> fertilizerTemplate = TSubclassOf<UPrimalItem>(fertilizerClassObj);

	// Get player position.
	FVector playerPos = PC->DefaultActorLocationField();
	// Get player team ID.
	int playerTeamId = PC->TargetingTeamField();

	TArray<AActor*> dungBeetleActors = TArray<AActor*>();
	TArray<AActor*> aberrantDungBeetleActors = TArray<AActor*>();
	if (config.SearchInDungBeetlesInventories)
	{
		// Search dung beetles in given radius.
		UClass* dungBeetleClassObj = MyBPLoadClass(FILLCROPS_DUNG_BEETLE_BP);
		if (dungBeetleClassObj != nullptr)
		{
			// TODO: Try to use UKismetSystemLibrary instead of UVictoryCore.
			//try { UKismetSystemLibrary::SphereOverlapActors_NEW(world, playerPos, radius, nullptr, dungBeetleClassObj, nullptr, &dungBeetleActors); }
			//catch (const std::exception&) {}
			TSubclassOf<AActor> dungBeetleSubClass = TSubclassOf<AActor>(dungBeetleClassObj);
			try { UVictoryCore::ServerOctreeOverlapActorsClass(&dungBeetleActors, world, playerPos, config.Radius, EServerOctreeGroup::DINOPAWNS_TAMED, dungBeetleSubClass, true); }
			catch (const std::exception&) {}
		}

		// Search aberrant dung beetles in given radius.
		UClass* aberrantDungBeetleClassObj = MyBPLoadClass(FILLCROPS_ABERRANT_DUNG_BEETLE_BP);
		if (aberrantDungBeetleClassObj != nullptr)
		{
			// TODO: Try to use UKismetSystemLibrary instead of UVictoryCore.
			//try { UKismetSystemLibrary::SphereOverlapActors_NEW(world, playerPos, radius, nullptr, aberrantDungBeetleClassObj, nullptr, &aberrantDungBeetleActors); }
			//catch (const std::exception&) {}
			TSubclassOf<AActor> aberrantDungBeetleSubClass = TSubclassOf<AActor>(aberrantDungBeetleClassObj);
			try { UVictoryCore::ServerOctreeOverlapActorsClass(&aberrantDungBeetleActors, world, playerPos, config.Radius, EServerOctreeGroup::DINOPAWNS_TAMED, aberrantDungBeetleSubClass, true); }
			catch (const std::exception&) {}
		}

		// Merge aberrant dung beetles with dung beetles actors.
		int nbAberrantDungBeetles = aberrantDungBeetleActors.Num();
		if (nbAberrantDungBeetles > 0)
			for (int i = 0; i < nbAberrantDungBeetles; i++)
				if (aberrantDungBeetleActors[i] != nullptr)
					dungBeetleActors.Add(aberrantDungBeetleActors[i]);
	}

	TArray<UPrimalItem*> playerFertilizers = TArray<UPrimalItem*>();
	if (config.SearchInPlayerInventory)
	{
		// Get player inventory.
		UPrimalInventoryComponent* playerInventory = PC->GetPlayerInventoryComponent();
		if (playerInventory == nullptr)
		{
			AShooterCharacter* playerCharacter = PC->LastControlledPlayerCharacterField().Get(false);
			if (playerCharacter != nullptr)
				playerInventory = playerCharacter->MyInventoryComponentField();
		}

		// Search all available fertilizers in player inventory.
		if (playerInventory != nullptr)
		{
			TArray<UPrimalItem*> pFertilizerItems;
			playerInventory->FindAllItemsOfType(&pFertilizerItems, fertilizerTemplate, true, true, false, false, false, false, false);
			int pNbFertilizers = (int)pFertilizerItems.Num();
			if (pNbFertilizers > 0)
				for (int l = 0; l < pNbFertilizers; l++)
					if (pFertilizerItems[l] != nullptr)
						AddToFertilizersList(&playerFertilizers, pFertilizerItems[l]);
		}
	}

	// Search all available fertilizers from dung beetles inventories.
	TArray<std::pair<UPrimalItem*, APrimalDinoCharacter*>> dungBeetleFertilizers = TArray<std::pair<UPrimalItem*, APrimalDinoCharacter*>>();
	int nbDungBeetles = dungBeetleActors.Num();
	if (nbDungBeetles > 0)
		for (int j = 0; j < nbDungBeetles; j++)
		{
			AActor* dungBeetle = dungBeetleActors[j];
			if (dungBeetle != nullptr)
				if (dungBeetle->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
				{
					int dungBeetleTeamId = dungBeetle->TargetingTeamField();
					if (dungBeetleTeamId >= 50000 && dungBeetleTeamId == playerTeamId) // If dung beetle belongs to player using the command.
						if (!dungBeetle->IsPendingKillPending())
						{
							APrimalDinoCharacter* dino = (APrimalDinoCharacter*)dungBeetle;
							UPrimalInventoryComponent* dinoInventory = dino->MyInventoryComponentField();
							if (dinoInventory != nullptr)
							{
								// Search fertilizer in current dung beetle inventory.
								TArray<UPrimalItem*> fertilizerItems;
								dinoInventory->FindAllItemsOfType(&fertilizerItems, fertilizerTemplate, true, true, false, false, false, false, false);
								int nbFertilizers = (int)fertilizerItems.Num();
								if (nbFertilizers > 0)
									for (int k = 0; k < nbFertilizers; k++)
										if (fertilizerItems[k] != nullptr)
											dungBeetleFertilizers.Add(std::pair<UPrimalItem*, APrimalDinoCharacter*>(fertilizerItems[k], dino));
							}
						}
				}
		}

	// If there is no available fertilizer.
	int playerFertilizersLen = playerFertilizers.Num();
	int dungBeetleFertilizersLen = dungBeetleFertilizers.Num();
	if (playerFertilizersLen <= 0 && dungBeetleFertilizersLen <= 0) 
	{
		FString Text = L"No fertilizer was found";
		if (config.SearchInPlayerInventory)
			Text = Text + (config.SearchInDungBeetlesInventories ? L" in your inventory or nearby Dung-Beetles." : L" in your inventory.");
		else
			Text = Text + (config.SearchInDungBeetlesInventories ? L" in nearby Dung-Beetles." : L".");
		PC->ClientServerChatDirectMessage(&Text, FLinearColor(255, 0, 0), false);
		return true; // Prevent normal chat execution by returning true.
	}

	int playerFertilizersIndex = 0;
	int dungBeetleFertilizersIndex = 0;
	TArray<AActor*> foundCropPlots = TArray<AActor*>();

	// TODO: Try to use UKismetSystemLibrary instead of UVictoryCore.
	//try { UKismetSystemLibrary::SphereOverlapActors_NEW(world, playerPos, radius, nullptr, APrimalStructureItemContainer_CropPlot::GetPrivateStaticClass(), nullptr, &foundCropPlots); }
	//catch (const std::exception&) {}
	TSubclassOf<AActor> cropPlotSubClass = TSubclassOf<AActor>(APrimalStructureItemContainer_CropPlot::GetPrivateStaticClass());
	try { UVictoryCore::ServerOctreeOverlapActorsClass(&foundCropPlots, world, playerPos, config.Radius, EServerOctreeGroup::STRUCTURES, cropPlotSubClass, true); }
	catch (const std::exception&) {}

	// If no crop plots found in radius.
	int missingFert = 0;
	int nbCropPlotsInRadius = foundCropPlots.Num();
	if (nbCropPlotsInRadius <= 0) 
	{
		FString Text = L"No available crop plot was found nearby.";
		PC->ClientServerChatDirectMessage(&Text, FLinearColor(255, 0, 0), false);
		return true; // Prevent normal chat execution by returning true.
	}

	// Search for crop plots that belongs to player.
	for (int m = 0; m < nbCropPlotsInRadius; m++)
	{
		AActor* current = foundCropPlots[m];
		if (current != nullptr && current->IsA(APrimalStructureItemContainer_CropPlot::GetPrivateStaticClass()) && !current->IsPendingKillPending())
		{
			APrimalStructureItemContainer_CropPlot* cropPlot = (APrimalStructureItemContainer_CropPlot*)current;
			int cropPlotTeamId = cropPlot->TargetingTeamField();
			if (cropPlotTeamId == playerTeamId)
			{
				UPrimalInventoryComponent* cropPlotInventory = cropPlot->MyInventoryComponentField();
				if (cropPlotInventory != nullptr)
				{
#if DEBUG
					FVector cropPlotPos = FVector(0.0F, 0.0F, 0.0F);
					USceneComponent* cropPlotSceneComp = cropPlot->RootComponentField();
					if (cropPlotSceneComp != nullptr)
						cropPlotSceneComp->GetWorldLocation(&cropPlotPos);
					unsigned int cropPlotId = cropPlot->StructureIDField();
#endif
					int maxInventoryItems = cropPlotInventory->GetMaxInventoryItems(false);
					int currentNumInventoryItems = cropPlotInventory->GetCurrentNumInventoryItems();
					if (currentNumInventoryItems < maxInventoryItems)
					{
#if DEBUG
						Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Empty slots found. CurrentNumInventoryItems=[" + std::to_string(currentNumInventoryItems) + "] MaxInventoryItems=[" + std::to_string(maxInventoryItems) + "]");
#endif
						int nbFertSlotsAvailable = maxInventoryItems - currentNumInventoryItems;
						int removedFerts = 0;
						for (int n = 0; n < nbFertSlotsAvailable; n++)
						{
							if (playerFertilizersIndex < playerFertilizersLen)
							{
								playerFertilizersIndex++;
								// Get player inventory.
								UPrimalInventoryComponent* playerInv = PC->GetPlayerInventoryComponent();
								if (playerInv == nullptr)
								{
									AShooterCharacter* playerCharacter = PC->LastControlledPlayerCharacterField().Get(false);
									if (playerCharacter != nullptr)
										playerInv = playerCharacter->MyInventoryComponentField();
								}
								// Use fertilizer from player inventory.
								if (playerInv != nullptr)
								{
									UPrimalItem* fertToUse = playerFertilizers[playerFertilizersIndex - 1];
									if (fertToUse != nullptr)
									{
#if DEBUG
										Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Trying to remove 1 fertilizer from player inventory. Index=[" + std::to_string(playerFertilizersIndex - 1) + "] Len=[" + std::to_string(playerFertilizersLen) + "]");
#endif
										// Remove fertilizer from player inventory.
										FItemNetID fertToUseId = fertToUse->ItemIDField();
										playerInv->RemoveItem(&fertToUseId, false, false, true, true);
										removedFerts++;
#if DEBUG
										Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Removed 1 fertilizer from player inventory.");
#endif
									}
								}
							}
							else if (dungBeetleFertilizersIndex < dungBeetleFertilizersLen)
							{
								dungBeetleFertilizersIndex++;
								// Get dung beetle inventory.
								APrimalDinoCharacter* currDungBeetle = dungBeetleFertilizers[dungBeetleFertilizersIndex - 1].second;
								if (currDungBeetle != nullptr)
								{
									// Get dung beetle inventory.
									UPrimalInventoryComponent* dungBeetleInv = currDungBeetle->MyInventoryComponentField();
									// Use fertilizer from dung beetle inventory.
									if (dungBeetleInv != nullptr)
									{
										UPrimalItem* fertToUse = dungBeetleFertilizers[dungBeetleFertilizersIndex - 1].first;
										if (fertToUse != nullptr)
										{
#if DEBUG
											Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Trying to remove 1 fertilizer from dung beetle inventory. Index=[" + std::to_string(dungBeetleFertilizersIndex - 1) + "] Len=[" + std::to_string(dungBeetleFertilizersLen) + "]");
#endif
											// Remove fertilizer from dung beetle inventory.
											FItemNetID fertToUseId = fertToUse->ItemIDField();
											dungBeetleInv->RemoveItem(&fertToUseId, false, false, true, true);
											removedFerts++;
#if DEBUG
											Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Removed 1 fertilizer from dung beetle inventory.");
#endif
										}
									}
								}
							}
							else
							{
								missingFert++;
#if DEBUG
								Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: No more fertilizer available for this crop plot.");
#endif
							}
						}
						int totalFertToAdd = (removedFerts > nbFertSlotsAvailable ? nbFertSlotsAvailable : removedFerts);
						if (totalFertToAdd > 0)
							AddFertilizerToInventory(cropPlotInventory, totalFertToAdd);
#if DEBUG
						Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Removed fertilizers from inventories [" + std::to_string(removedFerts) + "] Fertilizers added to crop plot [" + std::to_string(totalFertToAdd) + "].");
#endif
					}
				}
			}
		}
	}
	// Show final message to player.
	FString Text = L"Crop plots filled" + (missingFert > 0 ? L" (" + FString(std::to_string(missingFert)) + L" fertilizer were missing)." : L".");
	PC->ClientServerChatDirectMessage(&Text, FLinearColor(0, 255, 0), false);
	// Prevent normal chat execution by returning true.
	return true;
}

bool CheckFillCropsCommand(AShooterPlayerController* PC, FString* Message, EChatSendMode::Type Mode, bool spam_check, bool command_executed)
{
	static std::map<unsigned long long, long long> lastRunCommandTime = std::map<unsigned long long, long long>();

	if (spam_check || command_executed) // If the message didn't pass spam check or if the command was already processed.
		return false; // Give back execution by returning false.

	if (PC && Message && (Mode == EChatSendMode::Type::AllianceChat || Mode == EChatSendMode::Type::GlobalTribeChat || Mode == EChatSendMode::Type::GlobalChat || Mode == EChatSendMode::Type::LocalChat))
	{
		// Get player net ID.
		auto steamId = PC->GetUniqueNetIdAsUINT64();
		// Make local copy of the message.
		FString msg = FString(*Message);
		// Check for /fill command.
		if (msg.Len() > 1)
		{
			auto fillCropsConfig = GetFillCropsConfig();
			FString chatCmdTrigger = FString(fillCropsConfig.ChatTriger);
			if (msg.Compare(chatCmdTrigger, ESearchCase::IgnoreCase) == 0)
			{
				// Make sure player waits 5 seconds before using /fill command again.
				auto currTime = time(0);
				long long timeSinceLastChatCommand = -1i64;
				if (lastRunCommandTime.find(steamId) != lastRunCommandTime.end())
					timeSinceLastChatCommand = currTime - lastRunCommandTime[steamId];
				if (timeSinceLastChatCommand < 0i64 || timeSinceLastChatCommand > FILLCROPS_INTERVAL_DELAY)
				{
					lastRunCommandTime[steamId] = currTime;
					return ProcessFillCropsCommand(PC, fillCropsConfig); // Fill crops.
				}
				else
				{
					FString Text = L"Please wait " + FString(std::to_string(FILLCROPS_INTERVAL_DELAY)) + L" seconds before using " + chatCmdTrigger + L" command a second time.";
					PC->ClientServerChatDirectMessage(&Text, FLinearColor(255, 0, 0), false);
					return true; // Prevent normal chat execution by returning true.
				}
			}
		}
	}
	// Give back execution by returning false.
	return false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Log::Get().Init("AutoFillCrops");
		ArkApi::GetCommands().AddOnChatMessageCallback("CheckFillCropsCommand", &CheckFillCropsCommand);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		ArkApi::GetCommands().RemoveOnChatMessageCallback("CheckFillCropsCommand");
		break;
	}
	return TRUE;
}
