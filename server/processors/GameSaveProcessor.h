/*
 * GameSaveProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class PlayerColor;
class CGameHandler;

class GameSaveProcessor
{
	CGameHandler * gameHandler;

	int autosaveCount = 0;

	void performAutosave();
public:
	GameSaveProcessor(CGameHandler * gameHandler);

	void onNewDay();
	void onPlayerEndsTurn(PlayerColor player);
};
