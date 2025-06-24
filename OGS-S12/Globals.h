#pragma once

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBossesEnabled = true;
	bool bBotsEnabled = true;

	bool bUseLegacyAI_MANG = true; // Keep this on true bro, I am NOT doing this!

	bool LateGame = false;

	bool Arena = false;

	int MaxBotsToSpawn = 100;
	int MinPlayersForEarlyStart = 2;
}
