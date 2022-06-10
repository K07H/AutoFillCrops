#include "AutoFillCrops.h"

#pragma comment(lib, "ArkApi.lib")

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
	FString blueprintPath = L"Blueprint'/Game/PrimalEarth/CoreBlueprints/Items/Consumables/PrimalItemConsumable_Fertilizer_Compost.PrimalItemConsumable_Fertilizer_Compost'";
	UClass* bpUClass = MyBPLoadClass(&blueprintPath);
	if (bpUClass != nullptr)
	{
		for (int i = 0; i < amount; i++)
		{
			UPrimalItem* item = UPrimalItem::AddNewItem(TSubclassOf<UPrimalItem>(bpUClass), nullptr, false, true, 1.0F, false, 1, false, 1.0F, false, nullptr, 1.0F);
			if (item != nullptr)
				inventory->AddItemObject(item);
		}
	}
}

bool ProcessFillCropsCommand(AShooterPlayerController* PC, float radius)
{
	UWorld* world = ArkApi::GetApiUtils().GetWorld();
	if (world != nullptr)
	{
		// Get fertilizer template.
		FString fertilizerClassStr = L"Blueprint'/Game/PrimalEarth/CoreBlueprints/Items/Consumables/PrimalItemConsumable_Fertilizer_Compost.PrimalItemConsumable_Fertilizer_Compost'";
		UClass* fertilizerClassObj = MyBPLoadClass(&fertilizerClassStr);
		if (fertilizerClassObj == nullptr)
		{
			Log::GetLog()->error("ERROR: Failed to load fertilizer class for /fill command.");
			return true;
		}
		TSubclassOf<UPrimalItem> fertilizerTemplate = TSubclassOf<UPrimalItem>(fertilizerClassObj);

		// Get player position.
		FVector playerPos = PC->DefaultActorLocationField();
		// Get player team ID.
		int playerTeamId = PC->TargetingTeamField();
		
		// Search dung beetles in given radius.
		TArray<AActor*> dungBeetleActors = TArray<AActor*>();
		FString dungBeetleClassStr = L"Blueprint'/Game/PrimalEarth/Dinos/DungBeetle/DungBeetle_Character_BP.DungBeetle_Character_BP'";
		UClass* dungBeetleClassObj = MyBPLoadClass(&dungBeetleClassStr);
		if (dungBeetleClassObj != nullptr)
		{
			// TODO: Try to use UKismetSystemLibrary instead of UVictoryCore.
			//try { UKismetSystemLibrary::SphereOverlapActors_NEW(world, playerPos, radius, nullptr, dungBeetleClassObj, nullptr, &dungBeetleActors); }
			//catch (const std::exception&) {}
			TSubclassOf<AActor> dungBeetleSubClass = TSubclassOf<AActor>(dungBeetleClassObj);
			try { UVictoryCore::ServerOctreeOverlapActorsClass(&dungBeetleActors, world, playerPos, radius, EServerOctreeGroup::DINOPAWNS_TAMED, dungBeetleSubClass, true); }
			catch (const std::exception&) {}
		}

		// Search aberrant dung beetles in given radius.
		TArray<AActor*> aberrantDungBeetleActors = TArray<AActor*>();
		FString aberrantDungBeetleClassStr = L"Blueprint'/Game/PrimalEarth/Dinos/DungBeetle/DungBeetle_Character_BP_Aberrant.DungBeetle_Character_BP_Aberrant'";
		UClass* aberrantDungBeetleClassObj = MyBPLoadClass(&aberrantDungBeetleClassStr);
		if (aberrantDungBeetleClassObj != nullptr)
		{
			// TODO: Try to use UKismetSystemLibrary instead of UVictoryCore.
			//try { UKismetSystemLibrary::SphereOverlapActors_NEW(world, playerPos, radius, nullptr, aberrantDungBeetleClassObj, nullptr, &aberrantDungBeetleActors); }
			//catch (const std::exception&) {}
			TSubclassOf<AActor> aberrantDungBeetleSubClass = TSubclassOf<AActor>(aberrantDungBeetleClassObj);
			try { UVictoryCore::ServerOctreeOverlapActorsClass(&aberrantDungBeetleActors, world, playerPos, radius, EServerOctreeGroup::DINOPAWNS_TAMED, aberrantDungBeetleSubClass, true); }
			catch (const std::exception&) {}
		}

		// Merge aberrant dung beetles with dung beetles actors.
		int nbAberrantDungBeetles = aberrantDungBeetleActors.Num();
		if (nbAberrantDungBeetles > 0)
			for (int i = 0; i < nbAberrantDungBeetles; i++)
				if (aberrantDungBeetleActors[i] != nullptr)
					dungBeetleActors.Add(aberrantDungBeetleActors[i]);




		// Get player inventory.
		UPrimalInventoryComponent* playerInventory = PC->GetPlayerInventoryComponent();
		if (playerInventory == nullptr)
		{
			AShooterCharacter* playerCharacter = PC->LastControlledPlayerCharacterField().Get(false);
			if (playerCharacter != nullptr)
				playerInventory = playerCharacter->MyInventoryComponentField();
		}
		// Search all available fertilizers in player inventory.
		TArray<UPrimalItem*> playerFertilizers = TArray<UPrimalItem*>();
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
												dungBeetleFertilizers.Add(std::pair<UPrimalItem*, APrimalDinoCharacter*>(fertilizerItems[k], dino)); //AddToFertilizersList(&dungBeetleFertilizers, fertilizerItems[k]);
								}
							}
					}
			}

		// If we found some available fertilizers.
		int playerFertilizersLen = playerFertilizers.Num();
		int dungBeetleFertilizersLen = dungBeetleFertilizers.Num();
		if (playerFertilizersLen > 0 || dungBeetleFertilizersLen > 0)
		{
			int playerFertilizersIndex = 0;
			int dungBeetleFertilizersIndex = 0;
			TArray<AActor*> foundCropPlots = TArray<AActor*>();

			// TODO: Try to use UKismetSystemLibrary instead of UVictoryCore.
			//try { UKismetSystemLibrary::SphereOverlapActors_NEW(world, playerPos, radius, nullptr, APrimalStructureItemContainer_CropPlot::GetPrivateStaticClass(), nullptr, &foundCropPlots); }
			//catch (const std::exception&) {}
			TSubclassOf<AActor> cropPlotSubClass = TSubclassOf<AActor>(APrimalStructureItemContainer_CropPlot::GetPrivateStaticClass());
			try { UVictoryCore::ServerOctreeOverlapActorsClass(&foundCropPlots, world, playerPos, radius, EServerOctreeGroup::STRUCTURES, cropPlotSubClass, true); }
			catch (const std::exception&) {}

			// Search for crop plots that belongs to player.
			int missingFert = 0;
			int nbCropPlotsInRadius = foundCropPlots.Num();
			if (nbCropPlotsInRadius > 0)
			{
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
								USceneComponent* cropPlotSceneComp = cropPlot->RootComponentField();
								if (cropPlotSceneComp != nullptr)
								{
									FVector cropPlotPos = FVector(0.0F, 0.0F, 0.0F);
									cropPlotSceneComp->GetWorldLocation(&cropPlotPos);

									int currentNumInventoryItems = cropPlotInventory->GetCurrentNumInventoryItems();
									unsigned int cropPlotId = cropPlot->StructureIDField();

									bool playerFertAvailable = playerFertilizersIndex < playerFertilizersLen;
									bool dungBeetleFertAvailable = dungBeetleFertilizersIndex < dungBeetleFertilizersLen;
									if (playerFertAvailable || dungBeetleFertAvailable) // If we have some remaining fertilizer available.
									{
										if (currentNumInventoryItems < 10)
										{
#if DEBUG
											Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Empty slots found. CurrentNumInventoryItems=[" + std::to_string(currentNumInventoryItems) + "]");
#endif

											int nbFertSlotsAvailable = 10 - currentNumInventoryItems;
											//int addedFert = 0;
											int removedFerts = 0;
											//while (addedFert < nbFertSlotsAvailable)
											for (int n = 0; n < nbFertSlotsAvailable; n++)
											{
												if (playerFertAvailable)
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
															Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Trying to add 1 fertilizer from player inventory. Index=[" + std::to_string(playerFertilizersIndex - 1) + "] Len=[" + std::to_string(playerFertilizersLen) + "]");
#endif

															// Remove fertilizer from player inventory.
															FItemNetID fertToUseId = fertToUse->ItemIDField();
															playerInv->RemoveItem(&fertToUseId, false, false, true, true);
															removedFerts++;

#if DEBUG
															Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Added 1 fertilizer from player inventory.");
#endif
														}
													}
												}
												else if (dungBeetleFertAvailable)
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
																Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Trying to add 1 fertilizer from dung beetle inventory. Index=[" + std::to_string(dungBeetleFertilizersIndex - 1) + "] Len=[" + std::to_string(dungBeetleFertilizersLen) + "]");
#endif

																// Remove fertilizer from dung beetle inventory.
																FItemNetID fertToUseId = fertToUse->ItemIDField();
																dungBeetleInv->RemoveItem(&fertToUseId, false, false, true, true);
																removedFerts++;
#if DEBUG
																Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Added 1 fertilizer from dung beetle inventory.");
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
											Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: Total fertilizers added to crop plot is [" + std::to_string(totalFertToAdd) + "].");
#endif
										}
									}
									else
									{
#if DEBUG
										Log::GetLog()->debug("Crop plot [" + std::to_string(cropPlotId) + "] at [" + std::to_string(cropPlotPos.X) + "][" + std::to_string(cropPlotPos.Y) + "][" + std::to_string(cropPlotPos.Z) + "]: No more fertlizer available for this crop plot.");
#endif
										if (currentNumInventoryItems < 10)
											missingFert = missingFert + (10 - currentNumInventoryItems);
									}
								}
							}
						}
					}
				}
				FString Text = (missingFert > 0 ? L"Crop plots filled (" + FString(std::to_string(missingFert)) + L" fertilizer were missing to completely fill crop plots)." : L"Crop plots filled.");
				PC->ClientServerChatDirectMessage(&Text, (missingFert > 0 ? FLinearColor(255, 165, 0) : FLinearColor(0, 255, 0)), false);
			}
			else // No crop plots found in radius.
			{
				FString Text = L"No available crop plots found nearby.";
				PC->ClientServerChatDirectMessage(&Text, FLinearColor(255, 0, 0), false);
			}
		}
		else // No available fertilizer.
		{
			FString Text = L"No available fertilizer found in your inventory or nearby dung beetles.";
			PC->ClientServerChatDirectMessage(&Text, FLinearColor(255, 0, 0), false);
		}
	}
	return true; // Prevent normal chat execution by returning true
}

bool CheckFillCropsCommand(AShooterPlayerController* PC, FString* Message, EChatSendMode::Type Mode, bool spam_check, bool command_executed)
{
	static std::map<unsigned long long, long long> lastRunCommandTimeFive = std::map<unsigned long long, long long>();

	if (spam_check || command_executed) // If the message didn't pass spam check or if the command was already processed.
		return false; // Give back execution by returning false.

	if (PC && Message && (Mode == EChatSendMode::Type::AllianceChat || Mode == EChatSendMode::Type::GlobalTribeChat || Mode == EChatSendMode::Type::LocalChat || Mode == EChatSendMode::Type::GlobalChat))
	{
		// Get player net ID.
		auto steamId = PC->GetUniqueNetIdAsUINT64();
		// Make local copy of the message.
		FString msg = FString(*Message);
		// Check for /fill command.
		if (msg.Len() > 1)
		{
			auto fillCropsConfig = GetFillCropsConfig();
			if (msg.Compare(FString(fillCropsConfig.second), ESearchCase::IgnoreCase) == 0)
			{
				// Make sure player waits 5 seconds before using /fill command again.
				auto currTime = time(0);
				long long timeSinceLastChatCommandFive = -1i64;
				if (lastRunCommandTimeFive.find(steamId) != lastRunCommandTimeFive.end())
					timeSinceLastChatCommandFive = currTime - lastRunCommandTimeFive[steamId];
				if (timeSinceLastChatCommandFive < 0i64 || timeSinceLastChatCommandFive > 5i64)
				{
					lastRunCommandTimeFive[steamId] = currTime;
					return ProcessFillCropsCommand(PC, fillCropsConfig.first); // Fill crops.
				}
				else
				{
					FString Text = L"Please wait 5 seconds before using /fill command a second time.";
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
