#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Looting.h"

auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
auto Gamemode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

std::vector<class FactionBot*> FactionBots{};
class FactionBot
{
public:
	// The playercontroller of the bot
	ABP_PhoebePlayerController_C* PC;

	// The Pawn of the bot
	AFortPlayerPawnAthena* Pawn;

	// Are we ticking the bot?
	bool bTickEnabled = false;

	// So we can track the current tick that the bot is doing
	uint64_t tick_counter = 0;

	// Is the bot stressed, will be determined every tick.
	bool bIsStressed = false;

	// The current target that the bot is focusing on
	AActor* CurrentTarget = nullptr;

	// The current alertlevel of the bot, should have different behaviour for each one
	EAlertLevel AlertLevel = EAlertLevel::Unaware;

	// The bots should only have one weapon, so why not make it easier for us to switch back to it if we switch to pickaxe
	UFortWeaponItemDefinition* Weapon = nullptr;
	FGuid WeaponGuid;

	// So we can easily switch to the pickaxe when we need to
	FGuid PickaxeGuid;
	UFortWeaponMeleeItemDefinition* Pickaxe = nullptr;

	// The cid (pretty self explanatory)
	std::string CID = "";

	// The actual name of the bots class
	std::string Name = "";

	// The patrol path that the henchmen/bot is assigned to
	AFortAthenaPatrolPath* PatrolPath = nullptr;

	// The location of the current patrol point
	FVector CurrentPatrolPointLoc;

	// The current patrol point the bot is on
	int CurrentPatrolPoint = 0;

	// the amount of patrol points the bot patrol path has
	int MaxPatrolPoints = 0;

	// Are we patrolling?
	bool bIsPatrolling = false;

	// patrol points decremental
	bool bIsPatrollingBack = false;

	// bot is paused for some time for next patrol
	bool bIsWaitingForNextPatrol = false;

	float TimeWaitingForNextPatrol = 0.f;

public:
	FactionBot(AFortPlayerPawnAthena* Pawn)
	{
		this->Pawn = Pawn;
		PC = (ABP_PhoebePlayerController_C*)Pawn->Controller;
		FactionBots.push_back(this);
	}

public:
	// We shouldnt need to use this but we have it anyway
	FGuid GetItem(UFortItemDefinition* Def)
	{
		for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
				return PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid;
		}
		return FGuid();
	}

	// only really should be called when initialisint the bot
	void GiveItem(UFortItemDefinition* ODef, int Count = 1, bool equip = true)
	{
		UFortItemDefinition* Def = ODef;
		if (Def->GetName() == "AGID_Boss_Tina") {
			Def = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/Boss/WID_Boss_Tina.WID_Boss_Tina");
		}
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
		Item->OwnerInventory = PC->Inventory;

		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
		{
			Item->ItemEntry.LoadedAmmo = Looting::GetClipSize((UFortWeaponItemDefinition*)Def);
		}

		PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->Inventory->Inventory.ItemInstances.Add(Item);
		PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->Inventory->HandleInventoryLocalUpdate();
		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) && equip)
		{
			this->Weapon = (UFortWeaponItemDefinition*)ODef;
			this->WeaponGuid = GetItem(Def);
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Def, GetItem(Def));
		}
		if (Def->GetName() == "WID_Harvest_Pickaxe_Athena_C_T01") {
			this->Pickaxe = (UFortWeaponMeleeItemDefinition*)Def;
			this->PickaxeGuid = GetItem(Def);
		}
	}

	// set the currenttarget so that we can have the bot react appropriately to the target
	virtual void OnPerceptionSensed(AActor* SourceActor, FAIStimulus& Stimulus)
	{
		if (Stimulus.bSuccessfullySensed == 1 && PC->LineOfSightTo(SourceActor, FVector(), true) && Pawn->GetDistanceTo(SourceActor) < 5000)
		{
			CurrentTarget = SourceActor;
		}
	}
};

namespace BossesBTService_Patrolling {
	auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;

	inline FVector GetPatrolLocation(FactionBot* bot) {
		bot->MaxPatrolPoints = (bot->PatrolPath->PatrolPoints.Num() - 1);
		FVector Loc{};
		if (!bot || !bot->PatrolPath || bot->PatrolPath->PatrolPoints.Num() == 0)
			return Loc;

		if (bot->MaxPatrolPoints < 1)
		{
			Log("Not enough patrol points for " + bot->PC->GetFullName() + ", cancelling patrol.");
			bot->PatrolPath = nullptr;
			return Loc;
		}

		FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
		if (PatrolPointLoc.IsZero())
		{
			Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
		}
		else
		{
			auto BotPos = bot->Pawn->K2_GetActorLocation();
			auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

			bot->PC->SetControlRotation(TestRot);
			bot->PC->K2_SetActorRotation(TestRot, true);
			Loc = PatrolPointLoc;
		}

		Log("Started Patrolling " + std::to_string(bot->MaxPatrolPoints) + " Patrol Points for bot: " + bot->Name);
		bot->CurrentPatrolPointLoc = Loc;
		bot->bIsPatrolling = true;
		return Loc;
	}

	inline FVector DetermineNextPatrolPointLoc(FactionBot* bot) {
		FVector Loc{};
		if (!bot || !bot->PatrolPath || bot->PatrolPath->PatrolPoints.Num() == 0)
			return Loc;

		if (!bot->bIsPatrollingBack) {
			if ((bot->CurrentPatrolPoint + 1) >= bot->MaxPatrolPoints) {
				bot->bIsPatrollingBack = true;
			}
			bot->CurrentPatrolPoint = bot->CurrentPatrolPoint + 1;
			if (!bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]) {
				bot->bIsPatrollingBack = true;
			}
			else {
				FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
				if (PatrolPointLoc.IsZero())
				{
					Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
				}
				else
				{
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);
					Loc = PatrolPointLoc;
				}
			}
		}
		else {
			if ((bot->CurrentPatrolPoint - 1) <= 0) {
				bot->bIsPatrollingBack = false;
			}
			bot->CurrentPatrolPoint = bot->CurrentPatrolPoint - 1;
			if (!bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]) {
				bot->bIsPatrollingBack = false;
			}
			else {
				FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
				if (PatrolPointLoc.IsZero())
				{
					Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
				}
				else
				{
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);
					Loc = PatrolPointLoc;
				}
			}
		}

		return Loc;
	}
}

class BossesBTService_AIEvaluator {
public:
	// When stressed the bot will handle combat situations with players or playerbots differently
	bool IsStressed(AFortPlayerPawnAthena* Pawn, ABP_PhoebePlayerController_C* PC) {
		// If the bots shield is below 0 and they are threatened then they are stressed
		if (Pawn->GetShield() <= 0 && PC->CurrentAlertLevel == EAlertLevel::Threatened) {
			return true;
		}
		return false;
	}

public:
	void Tick(FactionBot* bot) {
		FVector BotPos = bot->Pawn->K2_GetActorLocation();

		bot->bIsStressed = IsStressed(bot->Pawn, bot->PC);

		if (!bot->bIsStressed && bot->PatrolPath && bot->CurrentPatrolPointLoc.IsZero() && !bot->bIsPatrolling) {
			BossesBTService_Patrolling::GetPatrolLocation(bot);
		}

		if (bot->bIsPatrolling && bot->bIsWaitingForNextPatrol && Statics->GetTimeSeconds(UWorld::GetWorld()) >= bot->TimeWaitingForNextPatrol) {
			auto BotPos = bot->Pawn->K2_GetActorLocation();
			auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentPatrolPointLoc);
			
			bot->PC->SetControlRotation(TestRot);
			bot->PC->K2_SetActorRotation(TestRot, true);

			bot->bIsWaitingForNextPatrol = false;
		}

		if (bot->PC->CurrentAlertLevel != EAlertLevel::Threatened && bot->PC->CurrentAlertLevel != EAlertLevel::Alerted
			&& bot->PatrolPath && bot->bIsPatrolling && !bot->bIsWaitingForNextPatrol && !bot->CurrentPatrolPointLoc.IsZero()) {
			float Distance = Math->Vector_Distance(BotPos, bot->CurrentPatrolPointLoc);

			if (Distance < 175.0f) {
				bot->bIsWaitingForNextPatrol = true;
				bot->TimeWaitingForNextPatrol = Statics->GetTimeSeconds(UWorld::GetWorld()) + Math->RandomFloatInRange(1.5f, 3.0f);
				bot->CurrentPatrolPointLoc = BossesBTService_Patrolling::DetermineNextPatrolPointLoc(bot);
			}
		}
	}
};

namespace Bosses {
	inline void TickBots()
	{
		auto block = [](FactionBot* bot, std::function<void(FactionBot* bot)> const& SetUnaware, bool Alerted, bool Threatened, bool LKP) {
			BossesBTService_AIEvaluator Evaluator;
			Evaluator.Tick(bot);

			if (!Threatened && !Alerted && bot->PatrolPath && bot->bIsPatrolling && !bot->bIsWaitingForNextPatrol && !bot->CurrentPatrolPointLoc.IsZero()) {
				bot->PC->MoveToLocation(bot->CurrentPatrolPointLoc, 25.f, true, false, false, true, nullptr, true);
				//bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
			}

			bot->tick_counter++;
		};
		auto SetUnaware = [](FactionBot* bot) {
			bot->PC->CurrentAlertLevel = EAlertLevel::Unaware;
			bot->PC->OnAlertLevelChanged(bot->AlertLevel, EAlertLevel::Unaware);
			bot->Pawn->PawnStopFire(0);
			bot->PC->StopMovement();
			if (bot->PatrolPath) {
				bot->bIsPatrolling = true;
			}
		};
		for (auto bot : FactionBots)
		{
			auto Alerted = bot->PC->CurrentAlertLevel == EAlertLevel::Alerted;
			auto Threatened = bot->PC->CurrentAlertLevel == EAlertLevel::Threatened;
			auto LKP = bot->PC->CurrentAlertLevel == EAlertLevel::LKP;
			block(bot, SetUnaware, Alerted, Threatened, LKP);
		}
	}
}