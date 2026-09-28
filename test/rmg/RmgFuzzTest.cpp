/*
 * RmgFuzzTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/CRmgTemplateStorage.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/mapping/CMap.h"

namespace
{
/// Runs a single random map generation attempt. Returns true if a map was produced.
bool tryGenerate(int seed, si8 playerCount)
{
	CMapGenOptions options;
	options.setHumanOrCpuPlayerCount(playerCount);

	if(options.getPossibleTemplates().empty())
		return false; // no template matches - nothing to fuzz

	CMapGenerator generator(options, nullptr, seed);
	auto map = generator.generate();
	return map != nullptr;
}
}

/// Regression fuzzing for random map generator crashes (#5849, #7500, #4621, #2492).
/// Disabled by default because a full run takes minutes; enable with
/// --gtest_also_run_disabled_tests --gtest_filter=RmgFuzzTest.* and optionally
/// set RMG_FUZZ_SEEDS to control the number of generation attempts per player count.
TEST(RmgFuzzTest, DISABLED_generateManySeeds)
{
	const char * seedsEnv = std::getenv("RMG_FUZZ_SEEDS");
	const int seedsPerPlayerCount = seedsEnv ? std::atoi(seedsEnv) : 10;

	

	int attempts = 0;
	int successes = 0;

	for(si8 players = 2; players <= 3; ++players)
	{
		for(int seed = 1; seed <= seedsPerPlayerCount; ++seed)
		{
			SCOPED_TRACE(::testing::Message() << "players=" << static_cast<int>(players) << " seed=" << seed);
			++attempts;

			try
			{
				if(tryGenerate(seed, players))
					++successes;
			}
			catch(const std::exception & e)
			{
				FAIL() << "RMG threw an exception: " << e.what();
			}
		}
	}

	EXPECT_GT(attempts, 0);
	EXPECT_EQ(successes, attempts) << "some RMG attempts failed";
}

/// Quick smoke check that RMG machinery works at all in the test environment.
TEST(RmgFuzzTest, generateSingleSmallMap)
{
	

	CMapGenOptions options;
	options.setHumanOrCpuPlayerCount(2);

	CMapGenerator generator(options, nullptr, 1);
	auto map = generator.generate();
	ASSERT_NE(map, nullptr);
	EXPECT_GT(map->width, 0);
}
