#pragma once
#include "framework.h"

namespace Net {
	__int64 AActorGetNetMode(AActor* a1)
	{
		return 1;
	}

	__int64 WorldGetNetMode(UWorld* a1)
	{
		return 1; // ENetMode::DedicatedServer or smth
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x3EB6780), AActorGetNetMode, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x45C9D90), WorldGetNetMode, nullptr);

		Log("Hooked Net!");
	}
}