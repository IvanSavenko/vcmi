/*
 * ModManagerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/ModManager.h"
#include "../../lib/modding/ModSnapshot.h"

TEST(ModManagerTest, gameplayModWithNonGameplayDependencyIsInvalid)
{
	ModManager mods(JsonNode(), true);

	EXPECT_EQ(mods.findNonGameplayDependency("test-invalid-dependency"), std::optional<TModID>("test-interface"));
	EXPECT_EQ(mods.findNonGameplayDependency("vcmi-test"), std::nullopt);
	EXPECT_EQ(mods.findNonGameplayDependency("test-interface"), std::nullopt);
}
