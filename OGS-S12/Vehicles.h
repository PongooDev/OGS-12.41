#pragma once
#include "framework.h"

namespace Vehicles {
	void ServerMove(AFortPhysicsPawn* Pawn, FReplicatedPhysicsPawnState InState)
	{
		UPrimitiveComponent* Mesh = (UPrimitiveComponent*)Pawn->RootComponent;

		if (Mesh) {
			InState.Rotation.X -= 2.5;
			InState.Rotation.Y /= 0.3;
			InState.Rotation.Z -= -2.0;
			InState.Rotation.W /= -1.2;

			FTransform Transform{};
			Transform.Translation = InState.Translation;
			Transform.Rotation = InState.Rotation;
			Transform.Scale3D = FVector{ 1, 1, 1 };

			Mesh->K2_SetWorldTransform(Transform, false, nullptr, true);
			Mesh->bComponentToWorldUpdated = true;
			Mesh->SetPhysicsLinearVelocity(InState.LinearVelocity, 0, FName());
			Mesh->SetPhysicsAngularVelocity(InState.AngularVelocity, 0, FName());
		}
	}

	void SpawnVehicles()
	{
		if (Globals::bEventEnabled)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		static bool bHasSpawnedSharkChopper = false;

		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaVehicleSpawner::StaticClass(), &Spawners);

		for (AActor* ActorSp : Spawners) {
			AFortAthenaVehicleSpawner* Spawner = Cast<AFortAthenaVehicleSpawner>(ActorSp);
			if (!Spawner) continue;

			std::string Name = Spawner->GetName();


			if (!Name.starts_with("Apollo_Hoagie_Spawner") && !Name.starts_with("Athena_Meatball_L_Spawner")) continue;


			AActor* Vehicle = SpawnActorClass<AActor>(Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation(), Spawner->GetVehicleClass());

			if (!bHasSpawnedSharkChopper && Name.starts_with("Apollo_Hoagie_Spawner")) {
				bHasSpawnedSharkChopper = true;

				AActor* Shark_Chopper = SpawnActorClass<AActor>(FVector(113665, -91120, -3065), {}, Spawner->GetVehicleClass());

				HookVTable(Shark_Chopper->VTable, 0xEE, ServerMove, nullptr);
			}

			HookVTable(Vehicle->VTable, 0xEE, ServerMove, nullptr);
		}

		Spawners.Free();
	}
}