/*
* ExchangeSwapTownHeroes.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExchangeSwapTownHeroes.h"
#include "ExecuteHeroChain.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/IGameSettings.h"
#include "../../../lib/GameLibrary.h"

namespace NK2AI
{

using namespace Goals;

ExchangeSwapTownHeroes::ExchangeSwapTownHeroes(
	const CGTownInstance * targetTown,
	const CGHeroInstance * garrisonHero,
	HeroLockedReason lockingReason)
	:ElementarGoal(Goals::EXCHANGE_SWAP_TOWN_HEROES), targetTown(targetTown), garrisonHero(garrisonHero), lockingReason(lockingReason)
{
}

std::vector<ObjectInstanceID> ExchangeSwapTownHeroes::getAffectedObjects() const
{
	std::vector<ObjectInstanceID> affectedObjects = { targetTown->id };

	if(targetTown->getGarrisonHero())
		affectedObjects.push_back(targetTown->getGarrisonHero()->id);

	if(targetTown->getVisitingHero())
		affectedObjects.push_back(targetTown->getVisitingHero()->id);

	return affectedObjects;
}

bool ExchangeSwapTownHeroes::isObjectAffected(ObjectInstanceID id) const
{
	return targetTown->id == id
		|| (targetTown->getVisitingHero() && targetTown->getVisitingHero()->id == id)
		|| (targetTown->getGarrisonHero() && targetTown->getGarrisonHero()->id == id);
}

std::string ExchangeSwapTownHeroes::toString() const
{
	return "Exchange and swap heroes of " + targetTown->getNameTextID();
}

bool ExchangeSwapTownHeroes::operator==(const ExchangeSwapTownHeroes & other) const
{
	return targetTown == other.targetTown;
}

void ExchangeSwapTownHeroes::accept(AIGateway * aiGw)
{
	if(!getGarrisonHero())
	{
		auto currentGarrisonHero = targetTown->getGarrisonHero();

		if(!currentGarrisonHero)
			throw cannotFulfillGoalException("Invalid configuration. There is no hero in town garrison.");

		// Issue #7847: extracting a hero from garrison increases the number of
		// wandering heroes. If the player already has the maximum allowed, the
		// server rejects the swap with 'Cannot move hero out of the garrison,
		// there are already N wandering heroes!'. Fail the goal gracefully so
		// the AI can re-plan instead of looping on a rejected request.
		int wanderingHeroes = aiGw->cc->getHeroCount(aiGw->playerID, false);
		int mapCap = LIBRARY->engineSettings()->getInteger(EGameSettings::HEROES_PER_PLAYER_ON_MAP_CAP);
		if(wanderingHeroes >= mapCap)
			throw cannotFulfillGoalException("Cannot move hero out of the garrison: wandering hero cap reached");

		aiGw->cc->swapGarrisonHero(targetTown);

		if(currentGarrisonHero != targetTown->getVisitingHero())
		{
			logAi->error("VisitingHero is empty, expected %s", currentGarrisonHero->getNameTextID());
			return;
		}

		aiGw->buildArmyIn(targetTown);
		aiGw->nullkiller->unlockHero(currentGarrisonHero);
		logAi->debug("Extracted hero %s from garrison of %s", currentGarrisonHero->getNameTextID(), targetTown->getNameTextID());

		return;
	}

	if(targetTown->getVisitingHero() && targetTown->getVisitingHero() != getGarrisonHero())
		aiGw->cc->swapGarrisonHero(targetTown);

	aiGw->makePossibleUpgrades(targetTown);
	aiGw->moveHeroToTile(targetTown->visitablePos(), HeroPtr(getGarrisonHero(), aiGw->cc.get()));

	auto upperArmy = targetTown->getUpperArmy();
	
	if(!targetTown->getGarrisonHero())
	{
		if (!getGarrisonHero()->canBeMergedWith(*targetTown))
		{
			while (upperArmy->stacksCount() != 0)
			{
				aiGw->cc->dismissCreature(upperArmy, upperArmy->Slots().begin()->first);
			}
		}
	}
	
	aiGw->cc->swapGarrisonHero(targetTown);

	if(lockingReason != HeroLockedReason::NOT_LOCKED)
	{
		aiGw->nullkiller->lockHero(getGarrisonHero(), lockingReason);
	}

	if(targetTown->getVisitingHero() && targetTown->getVisitingHero() != getGarrisonHero())
	{
		aiGw->nullkiller->unlockHero(targetTown->getVisitingHero());
		aiGw->makePossibleUpgrades(targetTown->getVisitingHero());
	}

	logAi->debug("Put hero %s to garrison of %s", getGarrisonHero()->getNameTextID(), targetTown->getNameTextID());
}

}
