/*
 * MapListProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class CMapInfo;
class ResourcePath;
enum class EResType;
VCMI_LIB_NAMESPACE_END

class MapListProcessor
{
	bool isMapSupported(const CMapInfo & info) const;

	std::unordered_set<ResourcePath> getFiles(const std::string & dirURI, EResType resType) const;

public:
	std::vector<std::shared_ptr<CMapInfo>> getAllMaps() const;
	std::vector<std::shared_ptr<CMapInfo>> getAllSaves() const;
	std::vector<std::shared_ptr<CMapInfo>> getAllCampaigns() const;

	/// Attempts to load map info from specified path. Returns nullptr on failure
	std::shared_ptr<CMapInfo> tryLoadMap(const std::string & mapName) const;

	MapListProcessor();
};
