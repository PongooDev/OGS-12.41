#pragma once
#include "framework.h"
#include "BotNames.h"

#include "PC.h"
#include "Quests.h"

static std::vector<UAthenaCharacterItemDefinition*> CIDs{};
static std::vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
static std::vector<UAthenaBackpackItemDefinition*> Backpacks{};
static std::vector<UAthenaGliderItemDefinition*> Gliders{};
static std::vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
inline std::vector<UAthenaDanceItemDefinition*> Dances{};

enum class EBotState : uint8 {
    Warmup,
    PreBus, // Dont handle this, just there to stop bots from killing eachother before bus enters dropzone
    Bus,
    Skydiving,
    Gliding,
    Landed,
    Fleeing,
    Looting,
    LookingForPlayers,
    MovingToSafeZone,
    Stuck
};

enum class EBotStrafeType {
    StrafeLeft,
    StrafeRight
};

std::vector<class PlayerBot*> PlayerBotArray{};
struct PlayerBot
{
public:
    // So we can track the current tick that the bot is doing
    uint64_t tick_counter = 0;

    // The Pawn of the bot
    AFortPlayerPawnAthena* Pawn = nullptr;

    // The playercontroller of the bot
    ABP_PhoebePlayerController_C* PC = nullptr;

    // The playerstate of the bot
    AFortPlayerStateAthena* PlayerState = nullptr;

    // The current botstate, has different behaviours for different states
    EBotState BotState = EBotState::Warmup;

    // The cached botstate, usually if a botstate needs to revert back to the previous one
    EBotState CachedBotState = EBotState::Warmup;

    // The building that the bot collided with (doesent even works smd)
    ABuildingActor* StuckBuildingActor = nullptr;

    // The displayname of the bot
    FString DisplayName = L"Bot";

    // Is the bot stressed, will be determined every tick.
    bool bIsStressed = false;

    // Is the bot dead, basically marking the bot to be removed from the playerbotarray later
    bool bIsDead = false;

    // Has the bot thanked the bus driver
    bool bHasThankedBusDriver = false;

    // Is the bot currently strafing (combat technique & Unstuck method)
    bool bIsCurrentlyStrafing = false;

    // The strafe type used by the bot, determines what direction
    EBotStrafeType StrafeType = EBotStrafeType::StrafeLeft;

    // When should the current strafe end?
    float StrafeEndTime = 0.0f;

public:
    void OnDied(AFortPlayerStateAthena* KillerState, AActor* DamageCauser, FName BoneName)
    {
        static auto Acc_DistanceShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_DistanceShot.AccoladeId_DistanceShot");
        static auto Acc_LongShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_051_LongShot.AccoladeId_051_LongShot");
        static auto Acc_LudicrousShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_052_LudicrousShot.AccoladeId_052_LudicrousShot");
        static auto Acc_ImpossibleShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_053_ImpossibleShot.AccoladeId_053_ImpossibleShot");
        static auto Acc_Headshot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_048_HeadshotElim.AccoladeId_048_HeadshotElim");

        static auto Acc_Survival_Bronze = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze");
        static auto Acc_Survival_Silver = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver");
        static auto Acc_Survival_Gold = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold");

        Log("Bot Died");

        bIsDead = true;

        if (!KillerState || !DamageCauser || !PlayerState || !Pawn)
            return;

        FDeathInfo& DeathInfo = PlayerState->DeathInfo;

        DeathInfo.bDBNO = Pawn->bWasDBNOOnDeath;
        DeathInfo.DeathLocation = Pawn->K2_GetActorLocation();
        DeathInfo.DeathTags = Pawn->DeathTags;
        DeathInfo.Downer = KillerState ? KillerState : nullptr;
        AFortPawn* KillerPawn = KillerState ? KillerState->GetCurrentPawn() : nullptr;
        DeathInfo.Distance = (KillerPawn && Pawn) ? KillerPawn->GetDistanceTo(Pawn) : 0.f;
        DeathInfo.FinisherOrDowner = KillerState ? KillerState : nullptr;
        DeathInfo.DeathCause = PlayerState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
        DeathInfo.bInitialized = true;
        PlayerState->OnRep_DeathInfo();

        bool bIsKillerABot = KillerState->bIsABot;

        if (!PC->Inventory)
            return;

        auto KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();

        if (KillerPC && KillerPC->IsA(AFortPlayerControllerAthena::StaticClass()) && !bIsKillerABot)
        {
            KillerState->KillScore++;

            Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination"));
            Quests::GiveAccolade(KillerPC, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));

            // giving assist accolade cuz idfk how to track assists
            Quests::GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_013_Assist.AccoladeId_013_Assist"));

            for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
            {
                ((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
                ((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
            }

            KillerState->ClientReportKill(PlayerState);
            KillerState->OnRep_Kills();

            /*auto KillerPawn = KillerPC->MyFortPawn;

            if (GameState->PlayersLeft && GameState->PlayerBotsLeft <= 1 && !GameState->IsRespawningAllowed(PlayerState)) // This crashes it lol
            {
                if (KillerPawn != Pawn)
                {
                    UFortWeaponItemDefinition* KillerWeaponDef = nullptr;

                    if (auto ProjectileBase = Cast<AFortProjectileBase>(DamageCauser))
                        KillerWeaponDef = ((AFortWeapon*)ProjectileBase->GetOwner())->WeaponData;
                    if (auto Weapon = Cast<AFortWeapon>(DamageCauser))
                        KillerWeaponDef = Weapon->WeaponData;

                    KillerPC->PlayWinEffects(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause, false);
                    KillerPC->ClientNotifyWon(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause);
                }

                KillerState->Place = 1;
                KillerState->OnRep_Place();
                GameState->WinningPlayerState = KillerState;
                GameState->WinningTeam = KillerState->TeamIndex;
                GameState->OnRep_WinningPlayerState();
                GameState->OnRep_WinningTeam();
                GameMode->EndMatch();
            }*/
        }

        if (bIsKillerABot) {
            Log("got to the end!");
            return;
        }

        if (!PC::bFirstElimTriggered) {
            Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_017_First_Elimination.AccoladeId_017_First_Elimination"));
            PC::bFirstElimTriggered = true;
        }

        float Distance = DeathInfo.Distance / 100.0f;

        if (Distance >= 100.0f && Distance < 150.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_DistanceShot); // 100-149m accolade
        }
        else if (Distance >= 150.0f && Distance < 200.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_LongShot); // 150-199m accolade
        }
        else if (Distance >= 200.0f && Distance < 250.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_LudicrousShot); // 200-249m accolade
        }
        else if (Distance >= 250.0f)
        {
            Quests::GiveAccolade(KillerPC, Acc_ImpossibleShot); // 250+m accolade
        }

        if (BoneName.ToString() == "head")
        {
            KillerPC->HeadShots++;
            Quests::GiveAccolade(KillerPC, Acc_Headshot); // headshot accolade

            if (KillerPC->HeadShots == 4) {
                Quests::GiveAccolade(KillerPC, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_049_Headhunter.AccoladeId_049_Headhunter"));
            }
        }

        if (KillerPC) {
            float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());

            if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) > KillerPC->LastKillTimeWindow) {
                KillerPC->KillsInKillTimeWindow = 0;
                KillerPC->LastRecordedKillsInKillTimeWindow = 0;
            }
            KillerPC->LastKillTimeWindow = CurrentTime + 10.f;
            KillerPC->KillsInKillTimeWindow++;
        }

        Log("Got to the end");
    }

public:
    void SetStuck(AActor* OtherActor, FHitResult& Hit) {
        if (BotState < EBotState::Landed)
            return;

        if (BotState == EBotState::Stuck)
            return;

        if (OtherActor && OtherActor->IsA(ABuildingSMActor::StaticClass()))
        {
            ABuildingSMActor* Actor = (ABuildingSMActor*)OtherActor;
            float Health = Actor->GetHealth();
            FFindFloorResult Res;

            if (Actor->bCanBeDamaged == 1 && Health > 1 && Health < 2500)
            {
                Pawn->CharacterMovement->K2_FindFloor(Pawn->CapsuleComponent->K2_GetComponentLocation(), &Res);
                if (Res.HitResult.Actor.Get() == OtherActor)
                    return;
                StuckBuildingActor = Actor;
            }
        }

        CachedBotState = BotState;
        BotState = EBotState::Stuck;
    }

public:
    // Give an item to the bot
    void GiveItemBot(UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0)
    {
        UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
        Item->OwnerInventory = PC->Inventory;
        Item->ItemEntry.LoadedAmmo = LoadedAmmo;
        PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PC->Inventory->Inventory.ItemInstances.Add(Item);
        PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
        PC->Inventory->HandleInventoryLocalUpdate();
    }

    FFortItemEntry* GetEntry(UFortItemDefinition* Def)
    {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
                return &PC->Inventory->Inventory.ReplicatedEntries[i];
        }

        return nullptr;
    }

    void Emote()
    {
        auto EmoteDef = Dances[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Dances.size() - 1)];
        if (!EmoteDef)
            return;

        static UClass* EmoteAbilityClass = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");

        FGameplayAbilitySpec Spec{};
        AbilitySpecConstructor(&Spec, reinterpret_cast<UGameplayAbility*>(EmoteAbilityClass->DefaultObject), 1, -1, EmoteDef);
        GiveAbilityAndActivateOnce(reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec);
    }

    void ForceStrafe(bool override) {
        if (!bIsCurrentlyStrafing && override)
        {
            bIsCurrentlyStrafing = true;
            if (UKismetMathLibrary::RandomBool()) {
                StrafeType = EBotStrafeType::StrafeLeft;
            }
            else {
                StrafeType = EBotStrafeType::StrafeRight;
            }
            StrafeEndTime = Statics->GetTimeSeconds(UWorld::GetWorld()) + Math->RandomFloatInRange(2.0f, 5.0f);
        }
        else
        {
            if (Statics->GetTimeSeconds(UWorld::GetWorld()) < StrafeEndTime)
            {
                if (StrafeType == EBotStrafeType::StrafeLeft) {
                    Pawn->AddMovementInput((Pawn->GetActorRightVector() * -1.0f), 1.5f, true);
                }
                else {
                    Pawn->AddMovementInput(Pawn->GetActorRightVector(), 1.5f, true);
                }
            }
            else
            {
                bIsCurrentlyStrafing = false;
            }
        }
    }

    void LookAt(AActor* Actor)
    {
        if (!Pawn || PC->GetFocusActor() == Actor)
            return;

        if (!Actor)
        {
            PC->K2_ClearFocus();
            return;
        }

        PC->K2_SetFocus(Actor);
    }

    bool IsPickaxeEquiped() {
        if (!Pawn || !Pawn->CurrentWeapon)
            return false;

        if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            return true;
        }
        return false;
    }

    bool HasGun()
    {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
            if (Entry.ItemDefinition) {
                std::string ItemName = Entry.ItemDefinition->Name.ToString();
                if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                    || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                    return true;
                    break;
                }
            }
        }
        return false;
    }

    void EquipPickaxe()
    {
        if (!Pawn || !Pawn->CurrentWeapon)
            return;

        if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
                {
                    Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid);
                    break;
                }
            }
        }
    }
};

class BotsBTService_AIEvaluator {
public:
    // When stressed the bot will handle combat situations with players or other bots differently
    bool IsStressed(AFortPlayerPawnAthena* Pawn, ABP_PhoebePlayerController_C* PC) {
        // If the bots health is 75 or below then they are stressed
        if (Pawn->GetHealth() <= 75) {
            return true;
        }
        return false;
    }

public:
    void Tick(PlayerBot* bot) {
        FVector BotPos = bot->Pawn->K2_GetActorLocation();

        if (bot->Pawn->bIsCrouched && (bot->tick_counter % 30) == 0) {
            bot->Pawn->UnCrouch(false);
        }

        bot->bIsStressed = IsStressed(bot->Pawn, bot->PC);

        if (bot->bIsCurrentlyStrafing) {
            bot->ForceStrafe(false);
        }
    }
};

namespace PlayerBots {
    void FreeDeadBots() {
        for (size_t i = 0; i < PlayerBotArray.size();)
        {
            if (PlayerBotArray[i]->bIsDead) {
                delete PlayerBotArray[i];
                PlayerBotArray.erase(PlayerBotArray.begin() + i);
                Log("Freed a dead bot from the array!");
            }
            else {
                ++i;
            }
        }
    }

    void SpawnPlayerBots(AActor* SpawnLocator)
    {
        if (!Globals::bBotsEnabled)
            return;

        static auto BotBP = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C");
        static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/Phoebe/BehaviorTrees/BT_Phoebe.BT_Phoebe");
        static UBlackboardData* BlackboardData = StaticLoadObject<UBlackboardData>("/Game/Athena/AI/Phoebe/BehaviorTrees/BB_Phoebe.BB_Phoebe");

        if (!BotBP || !BehaviorTree)
            return;

        PlayerBot* bot = new PlayerBot{};

        AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        static auto BotMutator = (AFortAthenaMutator_Bots*)GameMode->ServerBotManager->CachedBotMutator;

        bot->Pawn = BotMutator->SpawnBot(BotBP, SpawnLocator, SpawnLocator->K2_GetActorLocation(), SpawnLocator->K2_GetActorRotation(), false);

        if (!bot->Pawn || !bot->Pawn->Controller)
            return;

        bot->PC = Cast<ABP_PhoebePlayerController_C>(bot->Pawn->Controller);
        bot->PlayerState = Cast<AFortPlayerStateAthena>(bot->PC->PlayerState);

        if (!bot->PC || !bot->PlayerState)
            return;

        if (!CIDs.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(1)) { // use the random bool so that some bots will be defaults (more realistic)
            // as you could probably tell, i did not write this!
            auto CID = CIDs[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, CIDs.size() - 1)];
            if (CID->HeroDefinition)
            {
                if (CID->HeroDefinition->Specializations.IsValid())
                {
                    for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
                    {
                        UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
                        if (Spec)
                        {
                            for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
                            {
                                UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
                                if (Part)
                                {
                                    bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                                }
                            }
                        }
                    }
                }
            }
            if (CID) {
                bot->PC->CosmeticLoadoutBC.Character = CID;
            }
        }
        if (!Backpacks.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5)) { // less likely to equip than skin cause lots of ppl prefer not to use backpack
            auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
            for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
            {
                UCustomCharacterPart* Part = Backpack->CharacterParts[j];
                if (Part)
                {
                    bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                }
            }
        }
        if (!Gliders.empty()) {
            auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
            bot->PC->CosmeticLoadoutBC.Glider = Glider;
        }
        if (!Contrails.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.95)) {
            auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];
            bot->PC->CosmeticLoadoutBC.SkyDiveContrail = Contrail;
        }
        for (size_t i = 0; i < Dances.size(); i++)
        {
            bot->PC->CosmeticLoadoutBC.Dances.Add(Dances.at(i));
        }

        bot->Pawn->CosmeticLoadout = bot->PC->CosmeticLoadoutBC;
        bot->Pawn->OnRep_CosmeticLoadout();

        if (Pickaxes.empty()) {
            Log("Pickaxes array is empty!");
            UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
            if (PickDef) {
                bot->GiveItemBot(PickDef);
                auto Entry = bot->GetEntry(PickDef);
                bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
            }
            else {
                Log("Default Pickaxe dont exist!");
            }
        }
        else {
            auto PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)];
            if (!PickDef)
            {
                Log("Cooked!");
                UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
                if (PickDef) {
                    bot->GiveItemBot(PickDef);
                    auto Entry = bot->GetEntry(PickDef);
                    bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
                }
                else {
                    Log("Default Pickaxe dont exist!");
                }
            }
            else {
                if (PickDef && PickDef->WeaponDefinition)
                {
                    bot->GiveItemBot(PickDef->WeaponDefinition);
                }

                auto Entry = bot->GetEntry(PickDef->WeaponDefinition);
                bot->Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
            }
        }

        if (BotDisplayNames.size() != 0) {
            std::srand(static_cast<unsigned int>(std::time(0)));
            int randomIndex = std::rand() % BotDisplayNames.size();
            std::string rdName = BotDisplayNames[randomIndex];
            BotDisplayNames.erase(BotDisplayNames.begin() + randomIndex);

            int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
            std::wstring wideString(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);


            FString CVName = FString(wideString.c_str());
            GameMode->ChangeName(bot->PC, CVName, true);
            bot->DisplayName = CVName;

            bot->PlayerState->OnRep_PlayerName();
        }

        for (auto SkillSet : bot->PC->DigestedBotSkillSets)
        {
            if (!SkillSet)
                continue;

            if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
                bot->PC->CacheAimingDigestedSkillSet = AimingSkill;

            if (auto AttackingSkill = Cast<UFortAthenaAIBotAttackingDigestedSkillSet>(SkillSet))
                bot->PC->CacheAttackingSkillSet = AttackingSkill;

            if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
                bot->PC->CacheHarvestDigestedSkillSet = HarvestSkill;

            if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
                bot->PC->CacheInventoryDigestedSkillSet = InventorySkill;

            if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
                bot->PC->CacheLootingSkillSet = LootingSkill;

            if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
                bot->PC->CacheMovementSkillSet = MovementSkill;

            if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
                bot->PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

            if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
                bot->PC->CachePlayStyleSkillSet = PlayStyleSkill;
        }

        bot->PC->BehaviorTree = BehaviorTree;
        bot->PC->RunBehaviorTree(BehaviorTree);
        bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard);
        bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard1);

        static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
        static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
        bot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::Warmup);
        bot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Warmup);

        bot->PC->PathFollowingComponent->MyNavData = ((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData;
        bot->PC->PathFollowingComponent->OnNavDataRegistered(((UAthenaNavSystem*)UWorld::GetWorld()->NavigationSystem)->MainNavData);

        bot->PlayerState->OnRep_CharacterData();

        bot->Pawn->CapsuleComponent->SetGenerateOverlapEvents(true);
        bot->Pawn->CharacterMovement->bCanWalkOffLedges = true;

        bot->Pawn->SetMaxHealth(100);
        bot->Pawn->SetHealth(100);
        bot->Pawn->SetMaxShield(100);
        bot->Pawn->SetShield(0);

        PlayerBotArray.push_back(bot); // gotta do this so we can tick them all
        //Log("Bot Spawned With DisplayName: " + bot->DisplayName.ToString());
    }

    void Tick() {
        if (!PlayerBotArray.empty()) {
            if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.001f))
            {
                FreeDeadBots();
            }
        }
        else {
            return;
        }

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        for (auto bot : PlayerBotArray) {
            if (!bot || !bot->Pawn || !bot->PC || !bot->PlayerState)
                continue;

            if (bot->tick_counter <= 150) {
                bot->tick_counter++;
                continue;
            }

            if (bot->BotState > EBotState::Bus) {
                BotsBTService_AIEvaluator Evaluator;
                Evaluator.Tick(bot); // tick the evaluator after the bot is out of the bus so we dont mess up anything or cause potential crash
            }

            if (bot->BotState == EBotState::Warmup) {
                if (bot->tick_counter % 300 == 0) {
                    bot->Emote();
                }
                //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
            }
            else if (bot->BotState == EBotState::PreBus) {
                bot->Pawn->SetHealth(100);
                bot->Pawn->SetShield(100);
                if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
                {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }
            }
            else if (bot->BotState == EBotState::Bus) {
                bot->Pawn->SetShield(0);
                if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
                {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }

                if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.002f)) {
                    if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5f)) {
                        bot->bHasThankedBusDriver = true;
                        bot->PC->ThankBusDriver();
                    }
                    bot->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
                    bot->Pawn->BeginSkydiving(true);
                    bot->BotState = EBotState::Skydiving;
                }
            }
            else if (bot->BotState == EBotState::Skydiving) {

            }
            else if (bot->BotState == EBotState::MovingToSafeZone) {
                // when we have logic for the bots to look for players we can check the distance to the nearest player and if the distance is close enoug-
                // - we can fight that player/bot
                
                if (GameState && GameState->SafeZoneIndicator)
                {
                    bot->PC->MoveToLocation(GameState->SafeZoneIndicator->NextCenter, 3000, true, false, false, true, nullptr, true);
                }
                else
                {
                    // Proceed normal action
                }
            }
            else if (bot->BotState == EBotState::Stuck) {
                if (!bot->IsPickaxeEquiped()) {
                    bot->EquipPickaxe();
                }
                bot->Pawn->PawnStartFire(0);

                if (!bot->Pawn->bIsCrouched && Math->RandomBoolWithWeight(0.025f)) {
                    bot->Pawn->Crouch(false);
                }

                if (Math->RandomBoolWithWeight(0.05f)) {
                    bot->Pawn->UnCrouch(false);
                    bot->Pawn->Jump();
                }

                if (bot->StuckBuildingActor) {
                    if (bot->StuckBuildingActor->GetHealth() <= 1) {
                        bot->Pawn->PawnStopFire(0);
                        bot->StuckBuildingActor = nullptr;
                        bot->BotState = bot->CachedBotState;
                    }
                    else {
                        if (!bot->IsPickaxeEquiped()) {
                            bot->EquipPickaxe();
                        }
                        bot->LookAt(bot->StuckBuildingActor);
                        bot->Pawn->PawnStartFire(0);
                    }
                }
                else {
                    bot->ForceStrafe(true);
                    bot->BotState = bot->CachedBotState;
                }

                bot->Pawn->PawnStopFire(0);
            }

            bot->tick_counter++;
        }
    }
}