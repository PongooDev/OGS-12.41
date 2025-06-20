#pragma once

namespace Globals {
	bool bIsProdServer = false;

	bool bCreativeEnabled = false;
	bool bSTWEnabled = false;
	bool bEventEnabled = false;

	bool bBossesEnabled = true;
	bool bBotsEnabled = true;

	bool bUseLegacyAI_MANG = false; // Legacy is generally better at the moment, ill keep this in until the BT is better

	bool LateGame = false;

	bool Arena = false;

	int MaxBotsToSpawn = 100;
	int MinPlayersForEarlyStart = 2;

	bool bAllowBotsToBeOnPlayerTeam = true;
}
