#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Abilities.h"
#include "Quests.h"
#include "Net.h"
#include "Bots.h"
#include "Misc.h"

namespace GameMode {
	namespace Event {
		static inline UClass* Starter;
		static inline UObject* JerkyLoader;
	}

	bool ReadyToStartMatch(UObject* Context, FFrame& Stack, bool* Ret)
	{
		Stack.IncrementCode();
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)Context;
		if (!GameMode) {
			Log("ReadyToStartMatch: GameMode doesent exist!");
			return *Ret = false;
		}
		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		float CurrentTime = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
		float WarmupTime = 90.f;

		static bool bSetupPlaylist = false;
		static bool bInitialized = false;
		static bool bListening = false;

		if (!bSetupPlaylist) {
			bSetupPlaylist = true;
			UFortPlaylistAthena* Playlist = nullptr;
			if (Globals::bEventEnabled) {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/BuffetPlaylist/Playlist/Playlist_Buffet.Playlist_Buffet");
			}
			else {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
			}
			if (!Playlist) {
				Log("Playlist Not Found!");
				return *Ret = false;
			}

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->CurrentPlaylistName = Playlist->PlaylistName;
			GameState->CurrentPlaylistId = Playlist->PlaylistId;
			GameState->OnRep_CurrentPlaylistId();

			GameState->WarmupCountdownEndTime = CurrentTime + WarmupTime;
			GameMode->WarmupCountdownDuration = WarmupTime;
			GameState->WarmupCountdownStartTime = CurrentTime;
			GameMode->WarmupEarlyCountdownDuration = WarmupTime;

			GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
			GameMode->GameSession->MaxSpectators = 0;
			GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
			GameMode->GameSession->MaxSplitscreensPerConnection = 2;
			GameMode->GameSession->bRequiresPushToTalk = false;
			GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

			GameMode->ActorsToClear.Free();
			GameMode->bDisableGCOnServerDuringMatch = true;

			Misc::MaxPlayersOnTeam = Playlist->MaxSquadSize;
			Misc::NextIdx = Playlist->DefaultFirstTeam;

			for (auto& Level : Playlist->AdditionalLevels)
			{
				bool Success = false;
				ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString());
				FAdditionalLevelStreamed level{};
				level.bIsServerOnly = false;
				level.LevelName = Level.ObjectID.AssetPathName;

				Log("Loading level " + level.LevelName.ToString());

				GameState->AdditionalPlaylistLevelsStreamed.Add(level);
			}
			for (auto& Level : Playlist->AdditionalLevelsServerOnly)
			{
				bool Success = false;
				ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString());
				FAdditionalLevelStreamed level{};
				level.bIsServerOnly = true;
				level.LevelName = Level.ObjectID.AssetPathName;

				Log("Loading server level " + level.LevelName.ToString());

				GameState->AdditionalPlaylistLevelsStreamed.Add(level);
			}
			GameState->OnRep_AdditionalPlaylistLevelsStreamed();
			GameState->OnFinishedStreamingAdditionalPlaylistLevel();
			GameMode->HandleAllPlaylistLevelsVisible();

			ShowFoundation(StaticLoadObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.LF_Athena_POI_19x19_2"));
			ShowFoundation(StaticLoadObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head6_18"));
			ShowFoundation(StaticLoadObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head5_14"));
			ShowFoundation(StaticLoadObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head3_8"));
			ShowFoundation(StaticLoadObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head_2"));
			ShowFoundation(StaticLoadObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head4_11"));

			Log("Setup Playlist: " + Playlist->GetName());
		}

		if (!GameState->MapInfo) {
			return *Ret = false;
		}

		if (!bInitialized) {
			bInitialized = true;

			GameState->OnRep_CurrentPlaylistId();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->bAllowSpectateAfterDeath = true;
			GameMode->MinRespawnDelay = 5.0f;
			GameMode->WarmupRequiredPlayerCount = 1;

			if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
			{
				GameMode->AISettings = GameState->CurrentPlaylistInfo.BasePlaylist->AISettings;

				GameMode->ServerBotManager = BotManager;
				BotManager->CachedGameState = GameState;
				BotManager->CachedGameMode = GameMode;

				if (!GameMode->SpawningPolicyManager)
				{
					GameMode->SpawningPolicyManager = SpawnActor<AFortAthenaSpawningPolicyManager>({}, {});
				}
				GameMode->SpawningPolicyManager->GameModeAthena = GameMode;
				GameMode->SpawningPolicyManager->GameStateAthena = GameState;

				if (GameMode->ServerBotManager->CachedBotMutator) {
					BotMutator = GameMode->ServerBotManager->CachedBotMutator;
				}

				if (!BotMutator)
				{
					BotMutator = (AFortAthenaMutator_Bots*)GameMode->GetMutatorByClass(GameMode, AFortAthenaMutator_Bots::StaticClass());
				}

				if (!BotMutator)
				{
					BotMutator = (AFortAthenaMutator_Bots*)GameMode->GetMutatorByClass(GameMode->GameState, AFortAthenaMutator_Bots::StaticClass());
				}

				if (!BotMutator) {
					BotMutator = SpawnActor<AFortAthenaMutator_Bots>({});
				}

				BotManager->CachedBotMutator = BotMutator;
				BotMutator->CachedGameMode = GameMode;
				BotMutator->CachedGameState = GameState;

				GameMode->AIDirector = SpawnActor<AAthenaAIDirector>({});
				if (GameMode->AIDirector) {
					//Log("AIDirector!");
					GameMode->AIDirector->Activate();
				}
				else {
					Log("No AIDirector!");
				}

				if (!GameMode->AIGoalManager)
				{
					GameMode->AIGoalManager = SpawnActor<AFortAIGoalManager>({});
				}

				UAISystem::GetDefaultObj()->AILoggingVerbose();

				for (size_t i = 0; i < UObject::GObjects->Num(); i++)
				{
					UObject* Obj = UObject::GObjects->GetByIndex(i);
					if (Obj && Obj->IsA(UAthenaCharacterItemDefinition::StaticClass()))
					{
						std::string SkinsData = ((UAthenaCharacterItemDefinition*)Obj)->Name.ToString();

						if (SkinsData.contains("Athena_Commando") || SkinsData.contains("CID_Character") || !SkinsData.contains("CID_NPC") || !SkinsData.contains("CID_VIP") || !SkinsData.contains("CID_TBD"))
						{
							CIDs.push_back((UAthenaCharacterItemDefinition*)Obj);
						}
					}
					if (Obj && Obj->IsA(UAthenaBackpackItemDefinition::StaticClass()))
					{
						Backpacks.push_back((UAthenaBackpackItemDefinition*)Obj);
					}
					if (Obj && Obj->IsA(UAthenaPickaxeItemDefinition::StaticClass()))
					{
						Pickaxes.push_back((UAthenaPickaxeItemDefinition*)Obj);
					}
					if (Obj && Obj->IsA(UAthenaDanceItemDefinition::StaticClass()))
					{
						std::string EmoteData = ((UAthenaDanceItemDefinition*)Obj)->Name.ToString();

						if (EmoteData.contains("EID") || !EmoteData.contains("Sync") || !EmoteData.contains("Owned"))
						{
							Dances.push_back((UAthenaDanceItemDefinition*)Obj);
						}

					}
					if (Obj && Obj->IsA(UAthenaGliderItemDefinition::StaticClass()))
					{
						Gliders.push_back((UAthenaGliderItemDefinition*)Obj);
					}
				}

				Log("Initialised Bots!");
			}
			else
			{
				Log("BotManager is nullptr!");
			}

			Log("Initialized!");
		}

		if (!bListening) {
			bListening = true;

			FName GameNetDriver = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
			UWorld::GetWorld()->NetDriver = CreateNetDriver(UEngine::GetEngine(), UWorld::GetWorld(), GameNetDriver);

			UWorld::GetWorld()->NetDriver->World = UWorld::GetWorld();
			UWorld::GetWorld()->NetDriver->NetDriverName = GameNetDriver;

			FString Error = FString();
			FURL URL{};
			URL.Port = 7777;

			if (InitListen(UWorld::GetWorld()->NetDriver, UWorld::GetWorld(), URL, true, Error)) {
				Log("InitListen Successful!");
			}
			else {
				Log("InitListen Unsuccessful: " + Error.ToString());
			}

			SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());

			for (int i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++) {
				UWorld::GetWorld()->LevelCollections[i].NetDriver = UWorld::GetWorld()->NetDriver;
			}

			GameMode->bWorldIsReady = true;

			Log("Listening!");
			SetConsoleTitleA("OGS 12.41 | Listening");
		}

		if (GameState->PlayersLeft > 0) {
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &BuildingFoundations);
			UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);

			return *Ret = true;
		}
		else {
			GameState->WarmupCountdownEndTime = CurrentTime + WarmupTime;
			GameMode->WarmupCountdownDuration = WarmupTime;
			GameState->WarmupCountdownStartTime = CurrentTime;
			GameMode->WarmupEarlyCountdownDuration = WarmupTime;
		}

		return *Ret = false;
	}

	inline APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerController* Player, AActor* StartingLoc)
	{
		if (Player->Pawn)
			return 0;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Player;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		auto Transform = StartingLoc->GetTransform();
		auto Pawn = (AFortPlayerPawn*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
		Log("SpawnDefaultPawnFor: Spawned Pawn For: " + PlayerState->GetPlayerName().ToString());

		Abilities::InitAbilitiesForPlayer(PC);

		PC->XPComponent->bRegisteredWithQuestManager = true;
		PC->XPComponent->OnRep_bRegisteredWithQuestManager();

		PlayerState->SeasonLevelUIDisplay = PC->XPComponent->CurrentLevel;
		PlayerState->OnRep_SeasonLevelUIDisplay();

		PC->GetQuestManager(ESubGame::Athena)->InitializeQuestAbilities(PC->Pawn);

		UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
		PlayerState->OnRep_CharacterData();

		PlayerState->SquadId = PlayerState->TeamIndex - 3;
		PlayerState->OnRep_SquadId();

		FGameMemberInfo Member;
		Member.MostRecentArrayReplicationKey = -1;
		Member.ReplicationID = -1;
		Member.ReplicationKey = -1;
		Member.TeamIndex = PlayerState->TeamIndex;
		Member.SquadId = PlayerState->SquadId;
		Member.MemberUniqueId = PlayerState->UniqueId;

		GameState->GameMemberInfoArray.Members.Add(Member);
		GameState->GameMemberInfoArray.MarkItemDirty(Member);

		UAthenaPickaxeItemDefinition* PickDef;
		FFortAthenaLoadout& CosmecticLoadoutPC = PC->CosmeticLoadoutPC;
		PickDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : StaticLoadObject<UAthenaPickaxeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
		//UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
		if (PickDef) {
			Inventory::GiveItem(PC, PickDef->WeaponDefinition, 1, 0);
		}
		else {
			Log("Pick Doesent Exist!");
		}
		Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
		Pawn->OnRep_CosmeticLoadout();

		for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
		{
			if (GameMode->StartingItems[i].Count > 0)
			{
				Inventory::GiveItem(PC, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
			}
		}

		GameState->OnRep_SafeZoneIndicator();
		GameState->OnRep_SafeZonePhase();

		PlayerState->ForceNetUpdate();
		Pawn->ForceNetUpdate();
		PC->ForceNetUpdate();

		ApplyCharacterCustomization(PlayerState, Pawn);

		return Pawn;
		//return (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
	}

	static __int64 (*StartAircraftPhaseOG)(AFortGameModeAthena* GameMode, char a2) = nullptr;
	__int64 StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
	{
		for (auto FactionBot : FactionBots) // idek if im supposed to be setting these for the henchmens
		{
			static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
			static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
			FactionBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusLocked);
			FactionBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
		}

		for (auto PlayerBot : PlayerBotArray)
		{

			if (!Globals::LateGame) {
				auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
				auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusLocked);
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
			}
			else {
				auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
				auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
			}

			static auto Name4 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_JumpOffBus_ExecutionStatus"));
			static auto Name3 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));
			PlayerBot->PC->Blackboard->SetValueAsBool(Name3, true);
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name4, (uint8)EExecutionStatus::ExecutionAllowed);
			 
			if (!Globals::LateGame) {
				PlayerBot->BotState = EBotState::PreBus; // Proper!
			}
			else {
				PlayerBot->BotState = EBotState::Bus;
			}
		}

		return StartAircraftPhaseOG(GameMode, a2);
	}

	static inline void (*OriginalOnAircraftExitedDropZone)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
	void OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
	{
		Log("OnAircraftExitedDropZone!");

		if (Globals::bBotsEnabled) { // kick all bots out of the bus
			for (auto PlayerBot : PlayerBotArray) {
				if (PlayerBot->BotState == EBotState::Bus) {
					AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

					PlayerBot->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
					PlayerBot->Pawn->BeginSkydiving(true);
					PlayerBot->BotState = EBotState::Skydiving;
					Log("Kicked a bot!");
				}
			}
		}

		if (Globals::bEventEnabled)
		{
			UFunction* StartEventFunc = Event::JerkyLoader->Class->GetFunction("BP_Jerky_Loader_C", "startevent");

			float ToStart = 0.f;
			Event::JerkyLoader->ProcessEvent(StartEventFunc, &ToStart);
		}
		
		return OriginalOnAircraftExitedDropZone(GameMode, FortAthenaAircraft);
	}

	__int64 (*OnAircraftEnteredDropZoneOG)(AFortGameModeAthena*);
	__int64 OnAircraftEnteredDropZone(AFortGameModeAthena* a1)
	{
		Log("OnAircraftEnteredDropZone Called!");

		for (auto FactionBot : FactionBots)
		{
			static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
			static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
			FactionBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
			FactionBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);
		}

		if (!Globals::LateGame) {
			for (auto PlayerBot : PlayerBotArray)
			{
				static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
				static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
				PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);

				static auto Name9 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));

				PlayerBot->PC->Blackboard->SetValueAsBool(Name9, true);

				PlayerBot->BotState = EBotState::Bus;
			}
		}

		return OnAircraftEnteredDropZoneOG(a1);
	}

	void (*StormOG)(AFortGameModeAthena* GameMode, int32 ZoneIndex);
	void __fastcall Storm(AFortGameModeAthena* GameMode, int32 ZoneIndex)
	{
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		static bool First = true;

		if (Globals::LateGame && !First) //fixes crashing
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
			}
		}
		else if (!Globals::LateGame)
		{
			for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
			{
				Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
			}
		}

		static bool initstorm = false;

		if (Globals::LateGame && !initstorm)
		{
			initstorm = true;
			GameMode->SafeZonePhase = 4;
			GameState->SafeZonePhase = 4;
			GameState->SafeZonesStartTime = 0;
			ZoneIndex = 4;
			First = false;

			if (Globals::bBotsEnabled) {
				for (size_t i = 0; i < PlayerBotArray.size(); i++)
				{
					PlayerBotArray[i]->BotState = EBotState::Bus;
				}
			}
		}

		if (Globals::LateGame && initstorm)
		{
			int newPhase = GameState->SafeZonePhase + 1;

			GameMode->SafeZonePhase = newPhase;
			GameState->SafeZonePhase = newPhase;
		}

		if (Globals::bBotsEnabled && !Globals::LateGame) {
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				// Lets actually make sure the bot landed first before moving to zone
				if (PlayerBotArray[i]->BotState < EBotState::Landed)
					continue;
				Log("SafeZone!");
				PlayerBotArray[i]->BotState = EBotState::MovingToSafeZone; // i dont know the best way to get the bots to move to zone tbh
			}
		}

		return StormOG(GameMode, ZoneIndex);
	}

	void Hook() {
		//MH_CreateHook((LPVOID)(ImageBase + 0x4640A30), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);
		ExecHook(StaticLoadObject<UFunction>("/Script/Engine.GameMode.ReadyToStartMatch"), ReadyToStartMatch);
		//HookVTable(AFortGameModeBR::GetDefaultObj(), 0x101, ReadyToStartMatch, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F6250), SpawnDefaultPawnFor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F9BB0), StartAircraftPhase, (LPVOID*)&StartAircraftPhaseOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E07D0), OnAircraftExitedDropZone, (LPVOID*)&OriginalOnAircraftExitedDropZone);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E0730), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18FD350), Storm, (LPVOID*)&StormOG);

		Log("Gamemode Hooked!");
	}
}