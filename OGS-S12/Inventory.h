#pragma once
#include "framework.h"

namespace Inventory {
	bool CompareGuids(FGuid Guid1, FGuid Guid2) {
		if (Guid1.A == Guid2.A
			&& Guid1.B == Guid2.B
			&& Guid1.C == Guid2.C
			&& Guid1.D == Guid2.D) {
			return true;
		}
		else {
			return false;
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

	inline void UpdateInventory(AFortPlayerController* PC, FFortItemEntry& Entry)
	{
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid, Entry.ItemGuid))
			{
				PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	void GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
	{
		EEvaluateCurveTableResult Result;
		float OutXY = 0;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &OutXY, FString());
		if (!Def->MaxStackSize.Curve.CurveTable || OutXY <= 0)
			OutXY = Def->MaxStackSize.Value;
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
				if (CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->GetItemGuid(), guid))
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
			if (CompareGuids(Guid, Entry.ItemGuid))
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
			if (CompareGuids(Entry.ItemGuid, Guid))
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

			if (CompareGuids(ItemInstance->ItemEntry.ItemGuid, Guid))
				return ItemInstance;
		}

		return nullptr;
	}

	static void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid Guid)
	{
		Log("ServerExecuteInventoryItem Called!");
		if (!PC->MyFortPawn || !PC->Pawn)
			return;

		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Guid))
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

		return;
	}

	void Hook() {
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x20D, ServerExecuteInventoryItem, nullptr);

		Log("Inventory Hooked!");
	}
}