/*
 * GameSaveProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameSaveProcessor.h"

#include "../CGameHandler.h"

#include "../../lib/TextOperations.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapping/CMapHeader.h"

GameSaveProcessor::GameSaveProcessor(CGameHandler * gameHandler)
	: gameHandler(gameHandler)
{
}

void GameSaveProcessor::performAutosave()
{
	int frequency = static_cast<int>(settings["general"]["saveFrequency"].Integer());
	if(frequency > 0 && gameHandler->getDate() % frequency == 0)
	{
		bool usePrefix = settings["general"]["useSavePrefix"].Bool();
		std::string prefix = std::string();

		if(usePrefix)
		{
			prefix = settings["general"]["savePrefix"].String();
			if(prefix.empty())
			{
				std::string name = gameHandler->getMapHeader()->name.toString();
				int txtlen = TextOperations::getUnicodeCharactersCount(name);

				TextOperations::trimRightUnicode(name, std::max(0, txtlen - 15));
				std::string forbiddenChars("\\/:?\"<>| ");
				std::replace_if(name.begin(), name.end(), [&](char c) { return std::string::npos != forbiddenChars.find(c); }, '_' );

				prefix = name + "_" + gameHandler->getStartInfo()->startTimeIso8601 + "/";
			}
		}

		autosaveCount++;

		int autosaveCountLimit = settings["general"]["autosaveCountLimit"].Integer();
		if(autosaveCountLimit > 0)
		{
			gameHandler->save("Saves/Autosave/" + prefix + std::to_string(autosaveCount));
			autosaveCount %= autosaveCountLimit;
		}
		else
		{
			std::string stringifiedDate = std::to_string(gameHandler->getDate(Date::MONTH))
					+ std::to_string(gameHandler->getDate(Date::WEEK))
					+ std::to_string(gameHandler->getDate(Date::DAY_OF_WEEK));

			gameHandler->save("Saves/Autosave/" + prefix + stringifiedDate);
		}
	}
}

void GameSaveProcessor::onNewDay()
{
	if(settings["general"]["startTurnAutosave"].Bool())
	{
		performAutosave();
	}
}

void GameSaveProcessor::onPlayerEndsTurn(PlayerColor player)
{
	if (!gameHandler->getStartInfo()->playerInfos.at(player).isControlledByHuman())
		return;

	if(!settings["general"]["startTurnAutosave"].Bool())
	{
		performAutosave();
	}
}
