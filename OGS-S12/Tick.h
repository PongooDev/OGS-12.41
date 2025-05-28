#pragma once
#include "framework.h"
#include "Globals.h"
#include "GameMode.h"
#include "Bots.h"

namespace Tick {
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(ImageBase + 0x1023F60);

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (!Driver->ReplicationDriver)
		{
			Log("ReplicationDriver Doesent Exist!");
		}

		ServerReplicateActors(Driver->ReplicationDriver);

		if (GameState->GamePhase == EAthenaGamePhase::Warmup
			&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) >= Globals::MinPlayersForEarlyStart
			&& GameState->WarmupCountdownEndTime > UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 10.f) {

			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 10.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			static bool InitialisedPlayerStarts = false;
			if (!InitialisedPlayerStarts)
			{
				UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
				InitialisedPlayerStarts = true;
			}

			if (((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhase == EAthenaGamePhase::Warmup &&
				GameMode->AlivePlayers.Num() > 0
				&& (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < GameMode->GameSession->MaxPlayers
				&& GameMode->AliveBots.Num() < Globals::MaxBotsToSpawn
				&& GameState->WarmupCountdownEndTime > UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()))
			{
				if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05f))
				{
					AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

					if (SpawnLocator)
					{
						PlayerBots::SpawnPlayerBots(SpawnLocator);
					}
				}
			}
		}

		if (GameState->WarmupCountdownEndTime - UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup)
		{
			GameMode::StartAircraftPhase(GameMode, 0);
		}

		if (Globals::bBossesEnabled && !Globals::bEventEnabled && GameState->GamePhase > EAthenaGamePhase::Warmup)
		{
			Bosses::TickBots();
		}

		if (Globals::bBotsEnabled && !Globals::bEventEnabled) {
			PlayerBots::Tick();
		}

		return TickFlushOG(Driver, DeltaTime);
	}

	inline float GetMaxTickRate()
	{
		return 30.f;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x42C3ED0), TickFlush, (LPVOID*)&TickFlushOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x4576310), GetMaxTickRate, nullptr);

		Log("Tick Hooked!");
	}
}