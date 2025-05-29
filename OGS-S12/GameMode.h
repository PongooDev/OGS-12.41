#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Abilities.h"
#include "Bots.h"
#include "Quests.h"

namespace GameMode {
	uint8 NextIdx = 3;
	int CurrentPlayersOnTeam = 0;
	int MaxPlayersOnTeam = 1;

	inline bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameMode);
	inline bool ReadyToStartMatch(AFortGameModeAthena* GameMode)
	{
		ReadyToStartMatchOG(GameMode);

		static bool SetupServer = false;
		static bool ServerListening = false;

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (!SetupServer) {
			static bool LoadedPlaylist = false;
			if (!LoadedPlaylist) {
				LoadedPlaylist = true;
				UFortPlaylistAthena* Playlist;
				if (Globals::bCreativeEnabled) {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2");
				}
				else if (Globals::bEventEnabled) {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Music/Playlist_Music_High.Playlist_Music_High");
				}
				else {
					Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");
				}
				if (!Playlist) {
					Log("Could not find playlist!");
					return false;
				}
				else {
					Log("Found Playlist!");
				}

				GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
				GameState->CurrentPlaylistInfo.MarkArrayDirty();
				GameState->OnRep_CurrentPlaylistInfo();

				GameMode->CurrentPlaylistName = Playlist->PlaylistName;
				GameMode->WarmupRequiredPlayerCount = 1;

				NextIdx = Playlist->DefaultFirstTeam;
				MaxPlayersOnTeam = Playlist->MaxSquadSize;

				if (MaxPlayersOnTeam > 1)
				{
					GameMode->bDBNOEnabled = true;
					GameMode->bAlwaysDBNO = true;
					GameState->bDBNODeathEnabled = true;
					GameState->SetIsDBNODeathEnabled(true);
				}

				GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
				GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
				GameState->CurrentPlaylistId = Playlist->PlaylistId;
				GameState->CurrentPlaylistInfo.MarkArrayDirty();

				GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
				GameMode->GameSession->MaxSpectators = 0;
				GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
				GameMode->GameSession->MaxSplitscreensPerConnection = 2;
				GameMode->GameSession->bRequiresPushToTalk = false;
				GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

				auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
				auto DR = 90.f;

				GameState->WarmupCountdownEndTime = TS + DR;
				GameMode->WarmupCountdownDuration = DR;
				GameState->WarmupCountdownStartTime = TS;
				GameMode->WarmupEarlyCountdownDuration = DR;

				GameState->CurrentPlaylistId = Playlist->PlaylistId;
				GameState->OnRep_CurrentPlaylistId();

				GameState->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;
				GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
				GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
				GameState->OnRep_Aircraft();

				if (Globals::bEventEnabled) {
					Log("Event is loaded!");

					TArray<AActor*> BuildingFoundations;
					UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &BuildingFoundations);


					for (AActor*& BuildingFoundation : BuildingFoundations) {
						ABuildingFoundation* Foundation = (ABuildingFoundation*)BuildingFoundation;
						if (!Foundation) continue;

						if (BuildingFoundation->GetName().contains("Jerky") ||
							BuildingFoundation->GetName().contains("LF_Athena_POI_19x19")) {
							ShowFoundation((ABuildingFoundation*)BuildingFoundation);
						}
					}

					BuildingFoundations.Free();
				}
				else {
					ABuildingFoundation* BuildingFound = StaticFindObject<ABuildingFoundation>(L"/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.LF_Athena_POI_19x19_2");
					ShowFoundation((ABuildingFoundation*)BuildingFound);
				}

				Log("Setup Playlist!");
			}

			if (!GameState->MapInfo) {
				//Log("Map isnt fully loaded yet, ReadyToStartMatch return false!"); //this spams so why log 
				return false;
			}

			static bool InitialisedBots = false;

			if (!InitialisedBots) {
				if (Globals::bEventEnabled) {
					InitialisedBots = true;
				}
				else if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
				{
					GameMode->ServerBotManager = BotManager;
					BotManager->CachedGameState = GameState;
					BotManager->CachedGameMode = GameMode;

					BotMutator = SpawnActor<AFortAthenaMutator_Bots>({});
					BotManager->CachedBotMutator = BotMutator;
					BotMutator->CachedGameMode = GameMode;
					BotMutator->CachedGameState = GameState;

					AAthenaAIDirector* Director = SpawnActor<AAthenaAIDirector>({});
					GameMode->AIDirector = Director;
					//Director->Activate();

					AFortAIGoalManager* GoalManager = SpawnActor<AFortAIGoalManager>({});
					GameMode->AIGoalManager = GoalManager;

					InitialisedBots = true;
					Log("Initialised Bots!");
				}
				else
				{
					Log("BotManager is nullptr!");
				}
			}

			if (!ServerListening) {
				ServerListening = true;

				if (Globals::bBotsEnabled) {
					CIDs = GetAllObjectsOfClass<UAthenaCharacterItemDefinition>();
					Pickaxes = GetAllObjectsOfClass<UAthenaPickaxeItemDefinition>();
					Backpacks = GetAllObjectsOfClass<UAthenaBackpackItemDefinition>();
					Gliders = GetAllObjectsOfClass<UAthenaGliderItemDefinition>();
					Contrails = GetAllObjectsOfClass<UAthenaSkyDiveContrailItemDefinition>();
					Dances = GetAllObjectsOfClass<UAthenaDanceItemDefinition>();
				}

				GameState->OnRep_CurrentPlaylistId();
				GameState->OnRep_CurrentPlaylistInfo();

				FName NetDriverDef = UKismetStringLibrary::Conv_StringToName(FString(L"GameNetDriver"));

				UNetDriver* NetDriver = CreateNetDriver(UEngine::GetEngine(), UWorld::GetWorld(), NetDriverDef);
				NetDriver->NetDriverName = NetDriverDef;
				NetDriver->World = UWorld::GetWorld();

				FString Error;
				FURL url = FURL();
				url.Port = 7777;

				InitListen(NetDriver, UWorld::GetWorld(), url, false, Error);
				SetWorld(NetDriver, UWorld::GetWorld());

				UWorld::GetWorld()->NetDriver = NetDriver;

				for (size_t i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++) {
					UWorld::GetWorld()->LevelCollections[i].NetDriver = NetDriver;
				}

				SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());

				for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels.Num(); i++)
				{
					FVector Loc{};
					FRotator Rot{};
					bool bSuccess = false;
					((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i], Loc, Rot, &bSuccess, FString());
					FAdditionalLevelStreamed NewLevel{};
					NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i].ObjectID.AssetPathName;
					NewLevel.bIsServerOnly = false;
					GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
				}

				for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly.Num(); i++)
				{
					FVector Loc{};
					FRotator Rot{};
					bool bSuccess = false;
					((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i], Loc, Rot, &bSuccess, FString());
					FAdditionalLevelStreamed NewLevel{};
					NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i].ObjectID.AssetPathName;
					NewLevel.bIsServerOnly = true;
					GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
				}
				GameState->OnRep_AdditionalPlaylistLevelsStreamed();
				GameState->OnFinishedStreamingAdditionalPlaylistLevel();

				GameMode->bWorldIsReady = true;

				Log("Listening: 7777");
				SetConsoleTitleA("OGS 12.41 | Listening: 7777");
			}

			/*if (UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0) {
				std::cout << "Return true\n";
				return true;
			}*/
		}

		/*if (GameMode->bDelayedStart)
		{
			return false;
		}*/

		//std::cout << GameMode->GetMatchState().ToString() << "\n";

		/*if (GameMode->GetMatchState().ToString() == "WaitingToStart")
		{
			if (GameMode->NumPlayers + GameMode->NumBots > 0)
			{
				Log("Enough Players/Bots, Starting match!");
				return true;
			}
		}*/

		if (GameState->PlayersLeft > 0)
		{
			return true;
		}
		else
		{
			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 90.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		return false;
		//return UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0;
	}

	inline APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerController* Player, AActor* StartingLoc)
	{
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Player;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		auto Transform = StartingLoc->GetTransform();
		auto Pawn = (AFortPlayerPawn*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);

		Abilities::InitAbilitiesForPlayer(PC);

		PC->XPComponent->bRegisteredWithQuestManager = true;
		PC->XPComponent->OnRep_bRegisteredWithQuestManager();

		PlayerState->SeasonLevelUIDisplay = PC->XPComponent->CurrentLevel;
		PlayerState->OnRep_SeasonLevelUIDisplay();

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
			Log("Pick Does Exist!");
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

		return Pawn;
		//return (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
	}

	inline __int64 PickTeam(__int64 a1, unsigned __int8 a2, __int64 a3)
	{
		uint8 Ret = NextIdx;
		CurrentPlayersOnTeam++;

		if (CurrentPlayersOnTeam == MaxPlayersOnTeam)
		{
			NextIdx++;
			CurrentPlayersOnTeam = 0;
		}
		return Ret;
	};

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
			static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
			static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusLocked);
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);

			static auto Name4 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_JumpOffBus_ExecutionStatus"));
			static auto Name3 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));
			PlayerBot->PC->Blackboard->SetValueAsBool(Name3, true);
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name4, (uint8)EExecutionStatus::ExecutionAllowed);
		}

		if (Globals::bBotsEnabled) {
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				PlayerBotArray[i]->BotState = EBotState::PreBus; // Proper!
			}
		}

		return StartAircraftPhaseOG(GameMode, a2);
	}

	static inline void (*OriginalOnAircraftExitedDropZone)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
	void OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
	{
		Log("OnAircraftExitedDropZone!");

		if (Globals::bBotsEnabled) { // kick all bots out of the bus
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

				PlayerBotArray[i]->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
				PlayerBotArray[i]->Pawn->BeginSkydiving(true);
				PlayerBotArray[i]->BotState = EBotState::Skydiving;
			}
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

		for (auto PlayerBot : PlayerBotArray)
		{
			static auto Name1 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhaseStep"));
			static auto Name2 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_GamePhase"));
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name1, (uint8)EAthenaGamePhaseStep::BusFlying);
			PlayerBot->PC->Blackboard->SetValueAsEnum(Name2, (uint8)EAthenaGamePhase::Aircraft);

			static auto Name9 = UKismetStringLibrary::Conv_StringToName(TEXT("AIEvaluator_Global_IsInBus"));

			PlayerBot->PC->Blackboard->SetValueAsBool(Name9, true);
		}

		if (Globals::bBotsEnabled) {
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				PlayerBotArray[i]->BotState = EBotState::Bus;
			}
		}

		return OnAircraftEnteredDropZoneOG(a1);
	}

	void (*StormOG)(AFortGameModeAthena* GameMode, int32 ZoneIndex);
	void __fastcall Storm(AFortGameModeAthena* GameMode, int32 ZoneIndex)
	{
		for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
		{
			Quests::GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
		}

		if (Globals::bBotsEnabled) {
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				PlayerBotArray[i]->BotState = EBotState::MovingToSafeZone; // i dont know the best way to get the bots to move to zone tbh
			}
		}

		return StormOG(GameMode, ZoneIndex);
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x4640A30), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F6250), SpawnDefaultPawnFor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E6B20), PickTeam, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F9BB0), StartAircraftPhase, (LPVOID*)&StartAircraftPhaseOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E07D0), OnAircraftExitedDropZone, (LPVOID*)&OriginalOnAircraftExitedDropZone);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E0730), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18FD350), Storm, (LPVOID*)&StormOG);

		Log("Gamemode Hooked!");
	}
}