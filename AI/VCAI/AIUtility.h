#pragma once

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../../lib/Connection.h"
#include "../../lib/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CStopWatch.h"

/*
 * AIUtility.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

typedef const int3& crint3;
typedef const std::string& crstring;

const int HERO_GOLD_COST = 2500;
const int GOLD_MINE_PRODUCTION = 1000, WOOD_ORE_MINE_PRODUCTION = 2, RESOURCE_MINE_PRODUCTION = 1;
const int ACTUAL_RESOURCE_COUNT = 7;
const int ALLOWED_ROAMING_HEROES = 8;

//implementation-dependent
extern const double SAFE_ATTACK_CONSTANT;
extern const int GOLD_RESERVE;

//provisional class for AI to store a reference to an owned hero object
//checks if it's valid on access, should be used in place of const CGHeroInstance*

struct HeroPtr
{
	const CGHeroInstance *h;
	ObjectInstanceID hid;

public:
	std::string name;

	
	HeroPtr();
	HeroPtr(const CGHeroInstance *H);
	~HeroPtr();

	operator bool() const
	{
		return validAndSet();
	}

	bool operator<(const HeroPtr &rhs) const;
	const CGHeroInstance *operator->() const;
	const CGHeroInstance *operator*() const; //not that consistent with -> but all interfaces use CGHeroInstance*, so it's convenient

	const CGHeroInstance *get(bool doWeExpectNull = false) const;
	bool validAndSet() const;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & this->h & hid & name;
	}
};

enum BattleState
{
	NO_BATTLE,
	UPCOMING_BATTLE,
	ONGOING_BATTLE,
	ENDING_BATTLE
};

// AI lives in a dangerous world. CGObjectInstances under pointer may got deleted/hidden.
// This class stores object id, so we can detect when we lose access to the underlying object.
struct ObjectIdRef
{
	ObjectInstanceID id;

	const CGObjectInstance *operator->() const;
	operator const CGObjectInstance *() const;

	ObjectIdRef(ObjectInstanceID _id);
	ObjectIdRef(const CGObjectInstance *obj);

	bool operator<(const ObjectIdRef &rhs) const;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
};

struct TimeCheck
{
	CStopWatch time;
	std::string txt;
	TimeCheck(crstring TXT) : txt(TXT)
	{
	}

	~TimeCheck()
	{
        logAi->traceStream() << boost::format("Time of %s was %d ms.") % txt % time.getDiff();
	}
};

struct AtScopeExit
{
	std::function<void()> foo;
	AtScopeExit(const std::function<void()> &FOO) : foo(FOO)
	{}
	~AtScopeExit()
	{
		foo();
	}
};


class ObjsVector : public std::vector<ObjectIdRef>
{
private:
};

template<int id>
bool objWithID(const CGObjectInstance *obj)
{
        return obj->ID == id;
}

template <typename Container, typename Item>
bool erase_if_present(Container &c, const Item &item)
{
	auto i = std::find(c.begin(), c.end(), item);
	if (i != c.end())
	{
		c.erase(i);
		return true;
	}

	return false;
}

template <typename V, typename Item, typename Item2>
bool erase_if_present(std::map<Item,V> & c, const Item2 &item)
{
	auto i = c.find(item);
	if (i != c.end())
	{
		c.erase(i);
		return true;
	}
	return false;
}

template <typename Container, typename Pred>
void erase(Container &c, Pred pred)
{
	c.erase(boost::remove_if(c, pred), c.end());
}

template<typename T>
void removeDuplicates(std::vector<T> &vec)
{
	boost::sort(vec);
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

std::string strFromInt3(int3 pos);
void foreach_tile_pos(std::function<void(const int3& pos)> foo);
void foreach_tile_pos(CCallback * cbp, std::function<void(CCallback * cbp, const int3& pos)> foo); // avoid costly retrieval of thread-specific pointer
void foreach_neighbour(const int3 &pos, std::function<void(const int3& pos)> foo);
void foreach_neighbour(CCallback * cbp, const int3 &pos, std::function<void(CCallback * cbp, const int3& pos)> foo); // avoid costly retrieval of thread-specific pointer

int howManyTilesWillBeDiscovered(const int3 &pos, int radious, CCallback * cbp);
int howManyTilesWillBeDiscovered(int radious, int3 pos, crint3 dir);
void getVisibleNeighbours(const std::vector<int3> &tiles, std::vector<int3> &out);

bool canBeEmbarkmentPoint(const TerrainTile *t, bool fromWater);
bool isBlockedBorderGate(int3 tileToHit);
bool isReachable(const CGObjectInstance *obj);
bool isCloser(const CGObjectInstance *lhs, const CGObjectInstance *rhs);

bool isWeeklyRevisitable (const CGObjectInstance * obj);
bool shouldVisit (HeroPtr h, const CGObjectInstance * obj);

ui64 evaluateDanger(const CGObjectInstance *obj);
ui64 evaluateDanger(crint3 tile, const CGHeroInstance *visitor);
bool isSafeToVisit(HeroPtr h, crint3 tile);
bool boundaryBetweenTwoPoints (int3 pos1, int3 pos2, CCallback * cbp);

bool compareMovement(HeroPtr lhs, HeroPtr rhs);
bool compareHeroStrength(HeroPtr h1, HeroPtr h2);
bool compareArmyStrength(const CArmedInstance *a1, const CArmedInstance *a2);
ui64 howManyReinforcementsCanGet(HeroPtr h, const CGTownInstance *t);
int3 whereToExplore(HeroPtr h);
