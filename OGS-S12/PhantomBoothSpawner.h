#pragma once
#include "framework.h"

namespace PhantomBoothSpawner {
	std::vector<FVector> PhantomBoothLocations = {
		// Agency Booths
		{ 3481, 2394, -1917 },
		{ 7685, 5974, -1535 },

	};

	std::vector<FRotator> PhantomBoothRotations = {
		// Agency Booths
		{ 0, 89.999954, 0 },
		{ 0, 90.000420, 0 },

	};

	void SpawnBooths() {
		if (PhantomBoothLocations.size() != PhantomBoothRotations.size()) {
			Log("Sizes for spawndata dont match!");
			return;
		}

		static auto PhantomBooth = StaticLoadObject<UClass>("/Game/Athena/Items/EnvironmentalItems/HidingProps/Props/B_HidingProp_PhantomBooth.B_HidingProp_PhantomBooth_C");
		for (int i = 0; i < PhantomBoothLocations.size(); i++) {
			FVector Location = PhantomBoothLocations[i];
			FRotator Rotation = PhantomBoothRotations[i];

			if (PhantomBooth) {
				SpawnActor<AActor>(Location, Rotation, nullptr, PhantomBooth);
				Log("Spawned a PhantomBooth!");
			}
			else {
				Log("PhantomBooth does not exist!");
			}
		}
	}
}