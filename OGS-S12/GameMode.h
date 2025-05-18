#pragma once
#include "framework.h"
#include "Inventory.h"
#include "Abilities.h"

namespace GameMode {
	uint8 NextIdx = 3;
	int CurrentPlayersOnTeam = 0;
	int MaxPlayersOnTeam = 1;

	bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameMode);
	bool ReadyToStartMatch(AFortGameModeAthena* GameMode) {
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		static bool SetupPlaylist = false;
		if (!SetupPlaylist) {
			SetupPlaylist = true;
			UFortPlaylistAthena* Playlist;
			if (Globals::bCreativeEnabled) {
				Playlist = StaticLoadObject<UFortPlaylistAthena>("/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2");
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

			GameMode->bDBNOEnabled = Playlist->MaxTeamSize > 1;
			GameMode->bAlwaysDBNO = Playlist->MaxTeamSize > 1;
			GameState->bDBNODeathEnabled = Playlist->MaxTeamSize > 1;
			GameState->SetIsDBNODeathEnabled(Playlist->MaxTeamSize > 1);

			NextIdx = Playlist->DefaultFirstTeam;
			MaxPlayersOnTeam = Playlist->MaxSquadSize;

			GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
			GameMode->GameSession->MaxSpectators = 0;
			GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
			GameMode->GameSession->MaxSplitscreensPerConnection = 2;
			GameMode->GameSession->bRequiresPushToTalk = false;
			GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 120.f;

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

			GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;

			Log("Setup Playlist!");
		}

		if (!GameState->MapInfo) { // cant listen without map being fully loaded
			return false;
		}

		static bool InitialisedBots = false;

		if (!InitialisedBots) {
			if (Globals::bEventEnabled || !Globals::bBotsEnabled) {
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

				Globals::bBotsEnabled = BotMutator;
				InitialisedBots = true;
				Log("Initialised Bots!");
			}
			else
			{
				Log("BotManager is nullptr!");
			}
		}

		static bool Listening = false;
		if (!Listening) {
			Listening = true;

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

		if (GameState->PlayersLeft > 0)
		{
			return true;
		}
		else
		{
			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 120.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		return false;
	}

	inline APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerController* Player, AActor* StartingLoc)
	{
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Player;
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto Pawn = (AFortPlayerPawn*)PC->Pawn;

		FTransform Transform = StartingLoc->GetTransform();

		Abilities::InitAbilitiesForPlayer(PC);

		PC->XPComponent->bRegisteredWithQuestManager = true;
		PC->XPComponent->OnRep_bRegisteredWithQuestManager();

		PlayerState->SeasonLevelUIDisplay = PC->XPComponent->CurrentLevel;
		PlayerState->OnRep_SeasonLevelUIDisplay(); // ig this isnt in this season

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
		//Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
		//((AFortPlayerStateAthena*)PlayerState)->HeroType = PC->CosmeticLoadoutPC.Character->HeroDefinition;
		//((void (*)(APlayerState*, APawn*)) (ImageBase + 0x67b35f8))(PlayerState, Pawn);

		for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
		{
			if (GameMode->StartingItems[i].Count > 0)
			{
				Inventory::GiveItem(PC, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
			}
		}

		GameState->OnRep_SafeZoneIndicator();
		GameState->OnRep_SafeZonePhase();

		return (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
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

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x4640A30), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x18F6250), SpawnDefaultPawnFor, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x18E6B20), PickTeam, nullptr);

		Log("Gamemode Hooked!");
	}
}