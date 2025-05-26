#pragma once
#include "framework.h"

namespace Inventory {
	EFortQuickBars GetQuickBars(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
			return EFortQuickBars::Primary;

		return EFortQuickBars::Secondary;
	}

	bool IsPrimaryQuickbar(UFortItemDefinition* Def)
	{
		return Def->IsA(UFortConsumableItemDefinition::StaticClass()) || Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) || Def->IsA(UFortGadgetItemDefinition::StaticClass());
	}

	bool IsInventoryFull(AFortPlayerController* PC)
	{
		int ItemNb = 0;
		auto InstancesPtr = &PC->WorldInventory->Inventory.ItemInstances;
		for (int i = 0; i < InstancesPtr->Num(); i++)
		{
			if (InstancesPtr->operator[](i))
			{
				if (GetQuickBars(InstancesPtr->operator[](i)->ItemEntry.ItemDefinition) == EFortQuickBars::Primary)
				{
					ItemNb++;

					if (ItemNb >= 5)
					{
						break;
					}
				}
			}
		}

		return ItemNb >= 5;
	}

	inline int GetMagSize(UFortWeaponItemDefinition* Def)
	{
		if (!Def)
			return 0;

		UDataTable* Table = Def->WeaponStatHandle.DataTable;
		if (!Table)
			return 0;
		int Ret = 0;
		auto& RowMap = *(TMap<FName, void*>*)(__int64(Table) + 0x30);
		for (auto& Pair : RowMap)
		{
			if (Pair.Key().ToString() == Def->WeaponStatHandle.RowName.ToString())
			{
				Ret = ((FFortRangedWeaponStats*)Pair.Value())->ClipSize;
			}
		}
		return Ret;
	}

	inline int GetMaxStackSize(UFortItemDefinition* Def) // doesnt work with consumables for some reason
	{
		float Value = *(float*)(__int64(Def) + 0x0178);
		auto Table = Def->MaxStackSize.Curve.CurveTable;
		EEvaluateCurveTableResult Res;
		float Out;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Table, Def->MaxStackSize.Curve.RowName, 0, &Res, &Out, FString());
		if (!Table || Out <= 0)
			Out = Value;
		return Out;
	}

	//having 2 of these seems wrong
	float GetMaxStack(UFortItemDefinition* Def) //consumables
	{
		if (!Def->MaxStackSize.Curve.CurveTable)
			return Def->MaxStackSize.Value;
		EEvaluateCurveTableResult Result;
		float Ret;
		((UDataTableFunctionLibrary*)UDataTableFunctionLibrary::StaticClass()->DefaultObject)->EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &Ret, FString());
		return Ret;
	}


	inline void UpdateInventory(AFortPlayerController* PC, FFortItemEntry& Entry)
	{
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid == Entry.ItemGuid)
			{
				PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	void ModifyEntry(AFortPlayerControllerAthena* PC, FFortItemEntry& Entry)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid == Entry.ItemGuid)
			{
				PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	void UpdateLoadedAmmo(AFortPlayerController* PC, AFortWeapon* Weapon)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Weapon->ItemEntryGuid)
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].LoadedAmmo = Weapon->AmmoCount;
				UpdateInventory((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	void UpdateLoadedAmmo(AFortPlayerController* PC, AFortWeapon* Weapon, int AmountToAdd)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Weapon->ItemEntryGuid)
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].LoadedAmmo += AmountToAdd;
				ModifyEntry((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				UpdateInventory((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	inline void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
	{

		UFortWorldItem* Item = Cast<UFortWorldItem>(Def->CreateTemporaryItemInstanceBP(Count, 0));
		Item->SetOwningControllerForTemporaryItem(PC);
		Item->OwnerInventory = PC->WorldInventory;
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);
		PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->WorldInventory->HandleInventoryLocalUpdate();

	}

	void UpdateStack(AFortPlayerController* PC, bool Update, FFortItemEntry* EntryToUpdate = nullptr)
	{
		PC->WorldInventory->bRequiresLocalUpdate = true;
		PC->WorldInventory->HandleInventoryLocalUpdate();
		PC->HandleWorldInventoryLocalUpdate();
		PC->ClientForceUpdateQuickbar(EFortQuickBars::Primary);
		PC->ClientForceUpdateQuickbar(EFortQuickBars::Secondary);

		if (Update)
		{

			PC->WorldInventory->Inventory.MarkItemDirty(*EntryToUpdate);
		}
		else
		{
			PC->WorldInventory->Inventory.MarkArrayDirty();
		}
	}

	FFortItemEntry* GiveStack(AFortPlayerControllerAthena* PC, UFortItemDefinition* Def, int Count = 1, bool GiveLoadedAmmo = false, int LoadedAmmo = 0, bool Toast = false)
	{
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);

		Item->SetOwningControllerForTemporaryItem(PC);
		Item->OwnerInventory = PC->WorldInventory;
		Item->ItemEntry.ItemDefinition = Def;
		Item->ItemEntry.Count = Count;


		if (GiveLoadedAmmo)
		{
			Item->ItemEntry.LoadedAmmo = LoadedAmmo;
		}
		Item->ItemEntry.ReplicationKey++;

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);

		UpdateStack(PC, false);
		return &Item->ItemEntry;
	}

	void GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
	{
		EEvaluateCurveTableResult Result;
		float OutXY = 0;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &OutXY, FString());
		if (!Def->MaxStackSize.Curve.CurveTable || OutXY <= 0)
			OutXY = Def->MaxStackSize.Value;;
		FFortItemEntry* Found = nullptr;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
			{
				Found = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
				PC->WorldInventory->Inventory.ReplicatedEntries[i].Count += Count;
				if (PC->WorldInventory->Inventory.ReplicatedEntries[i].Count > OutXY)
				{
					PC->WorldInventory->Inventory.ReplicatedEntries[i].Count = OutXY;
				}
				//if (PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues[0].IntValue)
					//PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues[0].IntValue = false;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				UpdateInventory(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				PC->WorldInventory->HandleInventoryLocalUpdate();
				return;
			}
		}

		if (!Found)
		{
			GiveItem(PC, Def, Count, LoadedAmmo);
		}
	}

	void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count)
	{
		bool Remove = false;
		FGuid guid;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (Entry.ItemDefinition == Def)
			{
				Entry.Count -= Count;
				if (Entry.Count <= 0)
				{
					PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues.Free();
					PC->WorldInventory->Inventory.ReplicatedEntries.RemoveSingle(i);
					Remove = true;
					guid = Entry.ItemGuid;
				}
				else
				{
					PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
					UpdateInventory(PC, Entry);
					PC->WorldInventory->HandleInventoryLocalUpdate();
					return;
				}
				break;
			}
		}

		if (Remove)
		{
			for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
			{
				if (PC->WorldInventory->Inventory.ItemInstances[i]->GetItemGuid() == guid)
				{
					PC->WorldInventory->Inventory.ItemInstances.RemoveSingle(i);
					break;
				}
			}
		}

		PC->WorldInventory->Inventory.MarkArrayDirty();
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	inline void RemoveItem(AFortPlayerController* PC, FGuid Guid, int Count)
	{
		for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (Entry.ItemGuid == Guid)
			{
				RemoveItem(PC, Entry.ItemDefinition, Count);
				break;
			}
		}
	}

	inline FFortItemEntry* FindEntry(AFortPlayerController* PC, FGuid Guid)
	{
		for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (Entry.ItemGuid == Guid)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	FFortItemEntry* FindItemEntry(AFortPlayerController* PC, UFortItemDefinition* ItemDef)
	{
		if (!PC || !PC->WorldInventory || !ItemDef)
			return nullptr;
		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); ++i)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == ItemDef)
			{
				return &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			}
		}
		return nullptr;
	}

	UFortWorldItem* FindItemInstance(AFortInventory* inv, UFortItemDefinition* ItemDefinition)
	{
		auto& ItemInstances = inv->Inventory.ItemInstances;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (ItemInstance->ItemEntry.ItemDefinition == ItemDefinition)
				return ItemInstance;
		}

		return nullptr;
	}

	UFortWorldItem* FindItemInstance(AFortInventory* inv, const FGuid& Guid)
	{
		auto& ItemInstances = inv->Inventory.ItemInstances;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (ItemInstance->ItemEntry.ItemGuid == Guid)
				return ItemInstance;
		}

		return nullptr;
	}

	inline void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid Guid)
	{
		if (!PC->MyFortPawn || !PC->Pawn)
			return;

		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Guid)
			{
				UFortWeaponItemDefinition* DefToEquip = (UFortWeaponItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
				{
					DefToEquip = ((UFortGadgetItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->GetWeaponItemDefinition();
				}
				else if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass())) {
					auto DecoItemDefinition = (UFortDecoItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
					PC->MyFortPawn->PickUpActor(nullptr, DecoItemDefinition);
					PC->MyFortPawn->CurrentWeapon->ItemEntryGuid = Guid;
					static auto FortDecoTool_ContextTrapStaticClass = StaticLoadObject<UClass>("/Script/FortniteGame.FortDecoTool_ContextTrap");
					if (PC->MyFortPawn->CurrentWeapon->IsA(FortDecoTool_ContextTrapStaticClass))
					{
						reinterpret_cast<AFortDecoTool_ContextTrap*>(PC->MyFortPawn->CurrentWeapon)->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
					}
					return;
				}
				PC->MyFortPawn->EquipWeaponDefinition(DefToEquip, Guid);
				break;
			}
		}
	}

	inline void (*ServerHandlePickupOG)(AFortPlayerPawn* Pawn, AFortPickup* Pickup, float InFlyTime, FVector InStartDirection, bool bPlayPickupSound);
	inline void ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, float InFlyTime, const FVector& InStartDirection, bool bPlayPickupSound)
	{
		if (!Pickup || !Pawn || !Pawn->Controller || Pickup->bPickedUp)
			return;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

		UFortItemDefinition* Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
		FFortItemEntry* FoundEntry = nullptr;
		FFortItemEntry& PickupEntry = Pickup->PrimaryPickupItemEntry;
		float MaxStackSize = GetMaxStack(Def);
		bool Stackable = Def->IsStackable();
		UFortItemDefinition* PickupItemDef = PickupEntry.ItemDefinition;
		bool Found = false;
		FFortItemEntry* GaveEntry = nullptr;

		if (IsInventoryFull(PC))
		{
			if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);

				Pickup->PickupLocationData.bPlayPickupSound = true;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
			{
				if (Stackable)
				{
					for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
					{
						FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

						if (Entry.ItemDefinition == PickupItemDef)
						{
							Found = true;
							if ((MaxStackSize - Entry.Count) > 0)
							{
								Entry.Count += PickupEntry.Count;

								if (Entry.Count > MaxStackSize)
								{
									SpawnStack((APlayerPawn_Athena_C*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
									Entry.Count = MaxStackSize;
								}

								PC->WorldInventory->Inventory.MarkItemDirty(Entry);
							}
							else
							{
								if (IsPrimaryQuickbar(PickupItemDef))
								{
									GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
								}
							}
							break;
						}
					}
					if (!Found)
					{
						for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
						{
							if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Pawn->CurrentWeapon->GetInventoryGUID())
							{
								PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
								break;
							}
						}
						GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
					}

					Pickup->PickupLocationData.bPlayPickupSound = true;
					Pickup->PickupLocationData.FlyTime = 0.3f;
					Pickup->PickupLocationData.ItemOwner = Pawn;
					Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
					Pickup->PickupLocationData.PickupTarget = Pawn;
					Pickup->OnRep_PickupLocationData();

					Pickup->bPickedUp = true;
					Pickup->OnRep_bPickedUp();
					return;
				}

				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Pawn->CurrentWeapon->GetInventoryGUID())
					{
						PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
						break;
					}
				}
			}
		}

		if (!IsInventoryFull(PC))
		{
			if (Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

					if (Entry.ItemDefinition == PickupItemDef)
					{
						Found = true;
						if ((MaxStackSize - Entry.Count) > 0)
						{
							Entry.Count += PickupEntry.Count;

							if (Entry.Count > MaxStackSize)
							{
								SpawnStack((APlayerPawn_Athena_C*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
								Entry.Count = MaxStackSize;
							}

							PC->WorldInventory->Inventory.MarkItemDirty(Entry);
						}
						else
						{
							if (IsPrimaryQuickbar(PickupItemDef))
							{
								GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
							}
						}
						break;
					}
				}
				if (!Found)
				{
					GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
				}

				Pickup->PickupLocationData.bPlayPickupSound = true;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
			}
			else {
				GiveItem(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
			}
		}

		Pickup->PickupLocationData.bPlayPickupSound = true;
		Pickup->PickupLocationData.FlyTime = 0.3f;
		Pickup->PickupLocationData.ItemOwner = Pawn;
		Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
		Pickup->PickupLocationData.PickupTarget = Pawn;
		Pickup->OnRep_PickupLocationData();

		Pickup->bPickedUp = true;
		Pickup->OnRep_bPickedUp();
	}

	inline void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid ItemGuid, int Count, bool bTrash)
	{
		FFortItemEntry* Entry = FindEntry(PC, ItemGuid);
		SpawnPickup(Entry->ItemDefinition, Count, Entry->LoadedAmmo, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
		RemoveItem(PC, ItemGuid, Count);
	}

	__int64 (*OnReloadOG)(AFortWeapon* Weapon, int RemoveCount);
	__int64 OnReload(AFortWeapon* Weapon, int RemoveCount)
	{
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		auto Ret = OnReloadOG(Weapon, RemoveCount);
		auto WeaponDef = Weapon->WeaponData;
		if (!WeaponDef)
			return Ret;

		auto AmmoDef = WeaponDef->GetAmmoWorldItemDefinition_BP();
		if (!AmmoDef)
			return Ret;

		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)Weapon->GetOwner();
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

		if (!PC || !PC->Pawn || !PC->IsA(AFortPlayerControllerAthena::StaticClass()) || &PC->WorldInventory->Inventory == nullptr || GameState->GamePhase >= EAthenaGamePhase::EndGame)
			return Ret;

		if (PC->bInfiniteAmmo) {
			UpdateLoadedAmmo(PC, Weapon, RemoveCount);
			return Ret;
		}

		int AmmoCount = 0;
		FFortItemEntry* FoundEntry = nullptr;
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

			if (Entry.ItemDefinition == AmmoDef) {
				AmmoCount = Entry.Count;
				FoundEntry = &Entry;
				break;
			}
		}

		int AmmoToRemove = (RemoveCount < AmmoCount) ? RemoveCount : AmmoCount;

		if (AmmoToRemove > 0) {
			RemoveItem(PC, AmmoDef, AmmoToRemove);
			UpdateLoadedAmmo(PC, Weapon, AmmoToRemove);
		}


		if (WeaponDef == StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields"))
		{
			FGameplayEffectContextHandle Handle = PlayerState->AbilitySystemComponent->MakeEffectContext();
			FGameplayTag tag{};
			static auto name = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(TEXT("GameplayCue.Shield.PotionConsumed"));
			tag.TagName = name;

			PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(tag, FPredictionKey(), Handle);
			PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(tag, FPredictionKey(), Handle);
		}

		if (WeaponDef == StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit")) //doesnt work ;(
		{
			FGameplayEffectContextHandle Handle = PlayerState->AbilitySystemComponent->MakeEffectContext();
			FGameplayTag tag{};
			static auto name = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(TEXT("GameplayCue.Athena.Health.HealUsed"));
			tag.TagName = name;

			PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(tag, FPredictionKey(), Handle);
			PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(tag, FPredictionKey(), Handle);
		}

		PC->WorldInventory->bRequiresLocalUpdate = true;
		//PC->WorldInventory->Inventory.MarkItemDirty();
		PC->WorldInventory->HandleInventoryLocalUpdate();

		return Ret;
	}

	inline void (*NetMulticastDamageCuesOG)(AFortPlayerPawnAthena* Pawn, FAthenaBatchedDamageGameplayCues_Shared SharedData, FAthenaBatchedDamageGameplayCues_NonShared NonSharedData);
	inline void NetMulticastDamageCues(AFortPlayerPawnAthena* Pawn, FAthenaBatchedDamageGameplayCues_Shared SharedData, FAthenaBatchedDamageGameplayCues_NonShared NonSharedData)
	{
		if (!Pawn || Pawn->Controller->IsA(ABP_PhoebePlayerController_C::StaticClass()))
			return;

		if (Pawn->CurrentWeapon)
			UpdateLoadedAmmo((AFortPlayerController*)Pawn->Controller, ((AFortPlayerPawn*)Pawn)->CurrentWeapon);

		return NetMulticastDamageCuesOG(Pawn, SharedData, NonSharedData);
	}

	void Hook() {
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x20D, ServerExecuteInventoryItem, nullptr);

		HookVTable(APlayerPawn_Athena_C::GetDefaultObj(), 0x1EA, ServerHandlePickup, (LPVOID*)&ServerHandlePickupOG);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x21D, ServerAttemptInventoryDrop, nullptr);

		HookVTable(AFortPlayerPawnAthena::GetDefaultObj(), 0x119, NetMulticastDamageCues, (LPVOID*)&NetMulticastDamageCuesOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x260C490), OnReload, (LPVOID*)&OnReloadOG);

		Log("Inventory Hooked!");
	}
}