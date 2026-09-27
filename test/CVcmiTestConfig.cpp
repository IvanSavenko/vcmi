/*
 * CVcmiTestConfig.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CVcmiTestConfig.h"

#include "../lib/GameLibrary.h"
#include "../lib/CConfigHandler.h"
#include "../lib/texts/Languages.h"

void CVcmiTestConfig::SetUp()
{
	LIBRARY = new GameLibrary;
	// useTestPreset activates the core+vcmi+vcmi-test preset (config/testModSettings.json),
	// so tests are independent of the developer's active mods and never overwrite the real
	// modSettings.json. The vcmi-test mod supplies all fixtures and flips on the HOTA map
	// format needed by TinyH3MBuilder.
	LIBRARY->initializeFilesystem(false, /*useTestPreset*/ true);

	// Tests expect English texts: string comparisons in test assertions use
	// English strings from the base game data. Force English session language
	// so tests pass regardless of the language of the local HoMM3 install.
	Settings language = settings.write["session"]["language"];
	language->String() = "english";

	Settings encoding = settings.write["session"]["encoding"];
	encoding->String() = Languages::getLanguageOptions("english").encoding;

	LIBRARY->initializeLibrary();
}

void CVcmiTestConfig::TearDown()
{
	std::cout << "Ending global test tear-down." << std::endl;
}

