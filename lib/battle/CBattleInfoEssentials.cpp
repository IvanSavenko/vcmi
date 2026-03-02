/*
 * CBattleInfoEssentials.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInfoEssentials.h"

#include "../CStack.h"
#include "BattleInfo.h"
#include "CObstacleInstance.h"
#include "GameLibrary.h"
#include "IGameSettings.h"

#include "../callback/CallbackDefines.h"
#include "../constants/EntityIdentifiers.h"
#include "../entities/building/TownFortifications.h"
#include "../gameState/InfoAboutArmy.h"
#include "../mapObjects/CGTownInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

bool CBattleInfoEssentials::duringBattle() const
{
	return getBattle() != nullptr;
}

TerrainId CBattleInfoEssentials::battleTerrainType() const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getTerrainType();
}

BattleField CBattleInfoEssentials::battleGetBattlefieldType() const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getBattlefieldType();
}

int32_t CBattleInfoEssentials::battleGetEnchanterCounter(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getEnchanterCounter(side);
}

int32_t CBattleInfoEssentials::nextObstacleId() const
{
	int32_t maxId = -1;
	for (const auto & obstacle : getBattle()->getAllObstacles())
	{
		if (obstacle->uniqueID > maxId)
			maxId = obstacle->uniqueID;
	}
	return maxId + 1;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoEssentials::battleGetAllObstacles(std::optional<BattleSide> perspective) const
{
	std::vector<std::shared_ptr<const CObstacleInstance> > ret;
	THROW_IF_NOT_BATTLE();

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = std::make_optional(battleGetMySide());
	}
	else
	{
		if(!!getPlayerID() && *perspective != battleGetMySide())
			THROW_INVALID_CALL();
	}

	for(const auto & obstacle : getBattle()->getAllObstacles())
	{
		if(battleIsObstacleVisibleForSide(*(obstacle), *perspective))
			ret.push_back(obstacle);
	}

	return ret;
}

std::shared_ptr<const CObstacleInstance> CBattleInfoEssentials::battleGetObstacleByID(uint32_t ID) const
{
	std::shared_ptr<const CObstacleInstance> ret;

	THROW_IF_NOT_BATTLE();

	for(auto obstacle : getBattle()->getAllObstacles())
	{
		if(obstacle->uniqueID == ID)
			return obstacle;
	}

	THROW_INVALID_CALL();
}

bool CBattleInfoEssentials::battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	return side == BattleSide::ALL_KNOWING || coi.visibleForSide(side, battleHasNativeStack(side));
}

bool CBattleInfoEssentials::battleHasNativeStack(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();

	for(const auto * s : battleGetAllStacks())
	{
		if(s->unitSide() == side && s->isNativeTerrain(getBattle()->getTerrainType()))
			return true;
	}

	return false;
}

TStacks CBattleInfoEssentials::battleGetAllStacks(bool includeTurrets) const
{
	return battleGetStacksIf([=](const CStack * s)
	{
		return !s->isGhost() && (includeTurrets || !s->isTurret());
	});
}

battle::Units CBattleInfoEssentials::battleGetAllUnits(bool includeTurrets) const
{
	return battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return !unit->isGhost() && (includeTurrets || !unit->isTurret());
	});
}

TStacks CBattleInfoEssentials::battleGetStacksIf(const TStackFilter & predicate) const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getStacksIf(std::move(predicate));
}

battle::Units CBattleInfoEssentials::battleGetUnitsIf(const battle::UnitFilter & predicate)  const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getUnitsIf(predicate);
}

const battle::Unit * CBattleInfoEssentials::battleGetUnitByID(uint32_t ID) const
{
	THROW_IF_NOT_BATTLE();

	//TODO: consider using map ID -> Unit

	auto ret = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->unitId() == ID;
	});

	if(ret.empty())
		return nullptr;
	else
		return ret[0];
}

const battle::Unit * CBattleInfoEssentials::battleActiveUnit() const
{
	THROW_IF_NOT_BATTLE();
	auto id = getBattle()->getActiveStackID();
	if(id >= 0)
		return battleGetUnitByID(static_cast<uint32_t>(id));
	else
		return nullptr;
}

uint32_t CBattleInfoEssentials::battleNextUnitId() const
{
	return getBattle()->nextUnitId();
}

const CGTownInstance * CBattleInfoEssentials::battleGetDefendedTown() const
{
	THROW_IF_NOT_BATTLE();

	return getBattle()->getDefendedTown();
}

BattleSide CBattleInfoEssentials::battleGetMySide() const
{
	THROW_IF_NOT_BATTLE();
	if(!getPlayerID() || getPlayerID()->isSpectator())
		return BattleSide::ALL_KNOWING;
	if(*getPlayerID() == getBattle()->getSidePlayer(BattleSide::ATTACKER))
		return BattleSide::LEFT_SIDE;
	if(*getPlayerID() == getBattle()->getSidePlayer(BattleSide::DEFENDER))
		return BattleSide::RIGHT_SIDE;

	THROW_INVALID_CALL();
}

const CStack* CBattleInfoEssentials::battleGetStackByID(int ID, bool onlyAlive) const
{
	THROW_IF_NOT_BATTLE();

	auto stacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->unitId() == ID && (!onlyAlive || s->alive());
	});

	if(stacks.empty())
		return nullptr;
	else
		return stacks[0];
}

bool CBattleInfoEssentials::battleDoWeKnowAbout(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	auto p = battleGetMySide();
	return p == BattleSide::ALL_KNOWING || p == side;
}

si8 CBattleInfoEssentials::battleTacticDist() const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getTacticDist();
}

BattleSide CBattleInfoEssentials::battleGetTacticsSide() const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getTacticsSide();
}

int32_t CBattleInfoEssentials::battleGetRound() const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getRound();
}

const CGHeroInstance * CBattleInfoEssentials::battleGetFightingHero(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	if(side != BattleSide::DEFENDER && side != BattleSide::ATTACKER)
		THROW_INVALID_CALL();

	if(!battleDoWeKnowAbout(side))
		THROW_INVALID_CALL();

	return getBattle()->getSideHero(side);
}

const CArmedInstance * CBattleInfoEssentials::battleGetArmyObject(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	if(side != BattleSide::DEFENDER && side != BattleSide::ATTACKER)
		THROW_INVALID_CALL();

	if(!battleDoWeKnowAbout(side))
		THROW_INVALID_CALL();

	return getBattle()->getSideArmy(side);
}

InfoAboutHero CBattleInfoEssentials::battleGetHeroInfo(BattleSide side) const
{
	const auto * hero = getBattle()->getSideHero(side);
	if(!hero)
	{
		return InfoAboutHero();
	}
	InfoAboutHero::EInfoLevel infoLevel = battleDoWeKnowAbout(side) ? InfoAboutHero::EInfoLevel::DETAILED : InfoAboutHero::EInfoLevel::BASIC;
	return InfoAboutHero(hero, infoLevel);
}

int32_t CBattleInfoEssentials::battleCastSpells(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getCastSpells(side);
}

const IBonusBearer * CBattleInfoEssentials::getBonusBearer() const
{
	return getBattle()->getBonusBearer();
}

bool CBattleInfoEssentials::battleCanFlee(const PlayerColor & player) const
{
	THROW_IF_NOT_BATTLE();
	const BattleSide side = playerToSide(player);
	if(side == BattleSide::NONE)
		return false;

	const CGHeroInstance * myHero = battleGetFightingHero(side);

	//current player has no hero
	if(!myHero)
		return false;

	//eg. one of heroes is wearing shackles of war
	if(myHero->hasBonusOfType(BonusType::BATTLE_NO_FLEEING) && battleHasHero(otherSide(side)))
		return false;

	//cannot flee after casting spell in X first turns as attacker
	if(getBattle()->getRound() <= LIBRARY->engineSettings()->getInteger(EGameSettings::COMBAT_NO_SPELL_HIT_AND_RUN_ROUNDS)
		&& side == BattleSide::ATTACKER &&  battleHasHero(otherSide(side)) && getBattle()->getCastSpells(side) >= 1)
		return false;

	//we are besieged defender
	if(side == BattleSide::DEFENDER && getBattle()->getDefendedTown() != nullptr)
	{
		const auto * town = battleGetDefendedTown();
		if(!town->hasBuilt(BuildingSubID::ESCAPE_TUNNEL))
			return false;
	}

	return true;
}

BattleSide CBattleInfoEssentials::playerToSide(const PlayerColor & player) const
{
	THROW_IF_NOT_BATTLE();

	if(getBattle()->getSidePlayer(BattleSide::ATTACKER) == player)
		return BattleSide::ATTACKER;

	if(getBattle()->getSidePlayer(BattleSide::DEFENDER) == player)
		return BattleSide::DEFENDER;

	THROW_INVALID_CALL();
}

PlayerColor CBattleInfoEssentials::sideToPlayer(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getSidePlayer(side);
}

BattleSide CBattleInfoEssentials::otherSide(BattleSide side)
{
	if(side == BattleSide::ATTACKER)
		return BattleSide::DEFENDER;
	else
		return BattleSide::ATTACKER;
}

PlayerColor CBattleInfoEssentials::otherPlayer(const PlayerColor & player) const
{
	THROW_IF_NOT_BATTLE();

	auto side = playerToSide(player);
	if(side == BattleSide::NONE)
		return PlayerColor::CANNOT_DETERMINE;

	return getBattle()->getSidePlayer(otherSide(side));
}

bool CBattleInfoEssentials::playerHasAccessToHeroInfo(const PlayerColor & player, const CGHeroInstance * h) const
{
	THROW_IF_NOT_BATTLE();
	const auto side = playerToSide(player);
	if(side != BattleSide::NONE)
	{
		auto opponentSide = otherSide(side);
		if(getBattle()->getSideHero(opponentSide) == h)
			return true;
	}
	return false;
}

TownFortifications CBattleInfoEssentials::battleGetFortifications() const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getDefendedTown() ? getBattle()->getDefendedTown()->fortificationsLevel() : TownFortifications();
}

bool CBattleInfoEssentials::battleCanSurrender(const PlayerColor & player) const
{
	THROW_IF_NOT_BATTLE();
	const auto side = playerToSide(player);
	if(side == BattleSide::NONE)
		return false;
	bool iAmSiegeDefender = (side == BattleSide::DEFENDER && getBattle()->getDefendedTown() != nullptr);
	//conditions like for fleeing (except escape tunnel presence) + enemy must have a hero
	return battleCanFlee(player) && !iAmSiegeDefender && battleHasHero(otherSide(side));
}

bool CBattleInfoEssentials::battleHasHero(BattleSide side) const
{
	THROW_IF_NOT_BATTLE();
	return getBattle()->getSideHero(side) != nullptr;
}

EWallState CBattleInfoEssentials::battleGetWallState(EWallPart partOfWall) const
{
	THROW_IF_NOT_BATTLE();
	if(battleGetFortifications().wallsHealth == 0)
		return EWallState::NONE;

	return getBattle()->getWallState(partOfWall);
}

EGateState CBattleInfoEssentials::battleGetGateState() const
{
	THROW_IF_NOT_BATTLE();
	if(battleGetFortifications().wallsHealth == 0)
		return EGateState::NONE;

	return getBattle()->getGateState();
}

bool CBattleInfoEssentials::battleIsGatePassable() const
{
	THROW_IF_NOT_BATTLE();
	if(battleGetFortifications().wallsHealth == 0)
		return true;

	return battleGetGateState() == EGateState::OPENED || battleGetGateState() == EGateState::DESTROYED; 
}

PlayerColor CBattleInfoEssentials::battleGetOwner(const battle::Unit * unit) const
{
	THROW_IF_NOT_BATTLE();

	PlayerColor initialOwner = getBattle()->getSidePlayer(unit->unitSide());

	if(unit->isHypnotized())
		return otherPlayer(initialOwner);
	else
		return initialOwner;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetOwnerHero(const battle::Unit * unit) const
{
	THROW_IF_NOT_BATTLE();
	const auto side = playerToSide(battleGetOwner(unit));
	if(side == BattleSide::NONE)
		return nullptr;
	return getBattle()->getSideHero(side);
}

bool CBattleInfoEssentials::battleMatchOwner(const battle::Unit * attacker, const battle::Unit * defender, const boost::logic::tribool positivness) const
{
	THROW_IF_NOT_BATTLE();
	if(boost::logic::indeterminate(positivness))
		return true;
	else if(attacker->unitId() == defender->unitId())
		return (bool)positivness;
	else
		return battleMatchOwner(battleGetOwner(attacker), defender, positivness);
}

bool CBattleInfoEssentials::battleMatchOwner(const PlayerColor & attacker, const battle::Unit * defender, const boost::logic::tribool positivness) const
{
	THROW_IF_NOT_BATTLE();

	PlayerColor initialOwner = getBattle()->getSidePlayer(defender->unitSide());

	return boost::logic::indeterminate(positivness) || (attacker == initialOwner) == (bool)positivness;
}

VCMI_LIB_NAMESPACE_END
