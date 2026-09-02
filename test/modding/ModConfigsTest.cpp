/*
 * ModConfigsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <gtest/gtest.h>

#include "../../lib/GameLibrary.h"
#include "../../lib/modding/CModHandler.h"

/// Most mods declare no cursors at all, and an empty answer from one of them must leave what the
/// mods before it contributed alone - JsonUtils::merge clears its destination when handed a null.
TEST(ModConfigsTest, modsDeclaringNothingDoNotClearWhatCameBefore)
{
	JsonNode cursors = LIBRARY->modh->assembleModConfigs("cursors", JsonPath::builtin("config/cursors.json"));

	EXPECT_TRUE(cursors.Struct().count("combatPointer")) << "cursors declared by core were lost";
	EXPECT_TRUE(cursors.Struct().count("mapTurn1Aviate")) << "cursors declared by the vcmi mod were lost";
}
