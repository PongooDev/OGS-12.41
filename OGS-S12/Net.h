#pragma once
#include "framework.h"

struct LocRot {
	FVector Location;
	FRotator Rotation;
};
/*TArray<LocRot> PhantomBoothSpawners;
TArray<AActor*> PhantomBoothActors;

UClass* PhantomBoothClass;*/

namespace Net {
	enum class ENetMode
	{
		Standalone,
		DedicatedServer,
		ListenServer,
		Client,

		MAX,
	};

	ENetMode (*WorldGetNetModeOG)(UWorld*);
	ENetMode WorldGetNetMode(UWorld* a1)
	{
		std::string Name = a1->GetName();
		/*if (Name != "Apollo_Terrain" && Name != "Frontend") {
			Log(Name);
		}*/

		// Makes generators work for some reason
		/*if (Name.contains("Apollo_Terrain")) {
			return ENetMode::ListenServer;
		}*/

		ENetMode OriginalNetMode = WorldGetNetModeOG(a1);
		//Log("WorldNetMode: " + std::to_string(static_cast<int>(OriginalNetMode)));
		return ENetMode::DedicatedServer;
	}

	ENetMode (*AActorGetNetModeOG)(AActor*);
	ENetMode AActorGetNetMode(AActor* a1)
	{
		std::string Name = a1->GetName();

		/*if (Name.contains("Phantom")) {
			TArray<AActor*> PhantomBooths;
			auto BoothClass = a1->Class;
			PhantomBoothClass = BoothClass;
			Log(BoothClass->GetFullName());
			if (BoothClass) {
				TArray<AActor*> FoundBooths;
				auto* Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
				Statics->GetAllActorsOfClass(UWorld::GetWorld(), BoothClass, &FoundBooths);

				if (FoundBooths.Num() > PhantomBoothActors.Num()) {
					PhantomBoothSpawners.Clear();
					PhantomBoothActors = FoundBooths;

					for (auto* Booth : PhantomBoothActors) {
						if (!Booth) continue;

						LocRot locRot;
						locRot.Location = Booth->K2_GetActorLocation();
						locRot.Rotation = Booth->K2_GetActorRotation();
						Log("Booth Location:");
						Log("X: " + std::to_string(locRot.Location.X));
						Log("Y: " + std::to_string(locRot.Location.Y));
						Log("Z: " + std::to_string(locRot.Location.Z));
						Log("Booth Rotation:");
						Log("Pitch: " + std::to_string(locRot.Rotation.Pitch));
						Log("Yaw: " + std::to_string(locRot.Rotation.Yaw));
						Log("Roll: " + std::to_string(locRot.Rotation.Roll));

						FGameplayTag* Tag = (FGameplayTag*)((uintptr_t)Booth + 0xB0);
						FName TagName = Tag->TagName;
						Log(TagName.ToString());

						PhantomBoothSpawners.Add(locRot);
					}

					Log("Updated PhantomBoothSpawners with " + std::to_string(PhantomBoothSpawners.Num()) + " entries.");
				}
			}
			else {
				Log("PhantomBooth class is null!");
			}
		}*/

		a1->bAutoDestroyWhenFinished = false;
		a1->bNetTemporary = false;
		a1->bActorIsBeingDestroyed = false;

		//Log(Name);

		ENetMode OriginalNetMode = AActorGetNetModeOG(a1);
		//Log("AActorNetMode: " + std::to_string(static_cast<int>(OriginalNetMode)));
		return ENetMode::DedicatedServer;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0x45C9D90), WorldGetNetMode, (LPVOID*)&WorldGetNetModeOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x3EB6780), AActorGetNetMode, (LPVOID*)&AActorGetNetModeOG);

		Log("Hooked Net!");
	}
}