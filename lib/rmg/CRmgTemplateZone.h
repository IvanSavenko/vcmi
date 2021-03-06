
/*
 * CRmgTemplateZone.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "CMapGenerator.h"
#include "float3.h"
#include "../int3.h"

class CMapGenerator;
class CTileInfo;
class int3;
class CGObjectInstance;

namespace ETemplateZoneType
{
	enum ETemplateZoneType
	{
		PLAYER_START,
		CPU_START,
		TREASURE,
		JUNCTION
	};
}
class DLL_LINKAGE CTileInfo
{
public:

	CTileInfo();

	int getNearestObjectDistance() const;
	void setNearestObjectDistance(int value);
	bool isBlocked() const;
	bool shouldBeBlocked() const;
	bool isPossible() const;
	bool isFree() const;
	void setOccupied(ETileType::ETileType value);
	ETerrainType getTerrainType() const;
	void setTerrainType(ETerrainType value);

private:
	int nearestObjectDistance;
	ETileType::ETileType occupied;
	ETerrainType terrain;
};

/// The CRmgTemplateZone describes a zone in a template.
class DLL_LINKAGE CRmgTemplateZone
{
public:
	class DLL_LINKAGE CTownInfo
	{
	public:
		CTownInfo();

		int getTownCount() const; /// Default: 0
		void setTownCount(int value);
		int getCastleCount() const; /// Default: 0
		void setCastleCount(int value);
		int getTownDensity() const; /// Default: 0
		void setTownDensity(int value);
		int getCastleDensity() const; /// Default: 0
		void setCastleDensity(int value);

	private:
		int townCount, castleCount, townDensity, castleDensity;
	};

	CRmgTemplateZone();

	TRmgTemplateZoneId getId() const; /// Default: 0
	void setId(TRmgTemplateZoneId value);
	ETemplateZoneType::ETemplateZoneType getType() const; /// Default: ETemplateZoneType::PLAYER_START
	void setType(ETemplateZoneType::ETemplateZoneType value);
	int getSize() const; /// Default: 1
	void setSize(int value);
	boost::optional<int> getOwner() const;
	void setOwner(boost::optional<int> value);
	const CTownInfo & getPlayerTowns() const;
	void setPlayerTowns(const CTownInfo & value);
	const CTownInfo & getNeutralTowns() const;
	void setNeutralTowns(const CTownInfo & value);
	bool getTownsAreSameType() const; /// Default: false
	void setTownsAreSameType(bool value);
	const std::set<TFaction> & getTownTypes() const; /// Default: all
	void setTownTypes(const std::set<TFaction> & value);
	std::set<TFaction> getDefaultTownTypes() const;
	bool getMatchTerrainToTown() const; /// Default: true
	void setMatchTerrainToTown(bool value);
	const std::set<ETerrainType> & getTerrainTypes() const; /// Default: all
	void setTerrainTypes(const std::set<ETerrainType> & value);
	std::set<ETerrainType> getDefaultTerrainTypes() const;
	boost::optional<TRmgTemplateZoneId> getTerrainTypeLikeZone() const;
	void setTerrainTypeLikeZone(boost::optional<TRmgTemplateZoneId> value);
	boost::optional<TRmgTemplateZoneId> getTownTypeLikeZone() const;
	void setTownTypeLikeZone(boost::optional<TRmgTemplateZoneId> value);

	float3 getCenter() const;
	void setCenter(const float3 &f);
	int3 getPos() const;
	void setPos(const int3 &pos);

	void addTile (const int3 &pos);
	std::set<int3> getTileInfo () const;

	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addMonster(CMapGenerator* gen, int3 &pos, si32 strength);
	bool fill(CMapGenerator* gen);
	void createBorder(CMapGenerator* gen);
	bool crunchPath (CMapGenerator* gen, const int3 &src, const int3 &dst, TRmgTemplateZoneId zone);

	void addConnection(TRmgTemplateZoneId otherZone);
	std::vector<TRmgTemplateZoneId> getConnections() const;

private:
	//template info
	TRmgTemplateZoneId id;
	ETemplateZoneType::ETemplateZoneType type;
	int size;
	boost::optional<int> owner;
	CTownInfo playerTowns, neutralTowns;
	bool townsAreSameType;
	std::set<TFaction> townTypes;
	bool matchTerrainToTown;
	std::set<ETerrainType> terrainTypes;
	boost::optional<TRmgTemplateZoneId> terrainTypeLikeZone, townTypeLikeZone;

	//content info
	std::vector<int3> shape; //TODO: remove
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<CGObjectInstance*> objects;

	//placement info
	int3 pos;
	float3 center;
	std::set<int3> tileinfo; //irregular area assined to zone
	std::vector<TRmgTemplateZoneId> connections; //list of adjacent zones
	std::map<TRmgTemplateZoneId, bool> alreadyConnected; //TODO: allow multiple connections between two zones?

	bool pointIsIn(int x, int y);
	bool findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos);
	void checkAndPlaceObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos);
	void placeObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos);
	bool guardObject(CMapGenerator* gen, CGObjectInstance* object, si32 str);
};
