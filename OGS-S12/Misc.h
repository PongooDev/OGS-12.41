#pragma once
#include "framework.h"

namespace Misc {
    void nullFunc() {}

    int True() {
        return 1;
    }

    int False() {
        return 0;
    }

    static inline void (*KickPlayerOG)(AGameSession*, AController*);
    static void KickPlayer(AGameSession*, AController*) {
        return;
    }

    void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
    void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
    {
        return DispatchRequestOG(a1, a2, 3);
    }

    __int64 (*K2_DestroyActorOG)(AActor* This, __int64 a2);
    __int64 K2_DestroyActor(AActor* This, __int64 a2)
    {
        AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

        std::string Name = This->GetName();
        //Log(Name);

        if (GameState->GamePhase <= EAthenaGamePhase::Warmup) {
            if (Name.contains("BGA")) {
                int NumExisting = CountActorsWithName(This->Name, This->Class);

                if (NumExisting <= 1)
                {
                    //Log(Name);
                    DuplicateActor(This);
                }
            }
            else if (Name.contains("StaticGenerator")) {
                int NumExisting = CountActorsWithName(This->Name, This->Class);

                if (NumExisting <= 1)
                {
                    //Log(Name);
                    DuplicateActor(This);
                }
            }
        }

        __int64 result = K2_DestroyActorOG(This, a2);
        //Log(std::to_string(result));
        return result;
    }

    void LateGameAircraftThread(FVector BattleBusLocation)
    {
        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        Sleep(100);

        GameState->bAircraftIsLocked = false;

        GameMode->OnAircraftExitedDropZone(GameState->GetAircraft(0));

        GameState->GamePhase = EAthenaGamePhase::SafeZones;

        GameState->SafeZonesStartTime = 1;

    }

    void Hook() {
        MH_CreateHook((LPVOID)(ImageBase + 0x2D95E00), False, nullptr); // collectgarbage
        MH_CreateHook((LPVOID)(ImageBase + 0x4155600), KickPlayer, (LPVOID*)&KickPlayerOG); // Kickplayer
        MH_CreateHook((LPVOID)(ImageBase + 0x1E23840), False, nullptr);// change gamesession id
        MH_CreateHook((LPVOID)(ImageBase + 0x108D740), DispatchRequest, (LPVOID*)&DispatchRequestOG);

        MH_CreateHook((LPVOID)(ImageBase + 0x2DBCBA0), True, nullptr); // CanCreateContext

        MH_CreateHook((LPVOID)(ImageBase + 0x3ca10c0), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x2d95e00), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x3262100), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x1e23840), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x2d95dc0), nullFunc, nullptr);

        MH_CreateHook((LPVOID)(ImageBase + 0x7F0220), K2_DestroyActor, (LPVOID*)&K2_DestroyActorOG);

        Log("Misc Hooked!");
    }
}