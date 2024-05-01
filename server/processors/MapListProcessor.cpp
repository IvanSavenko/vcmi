/*
 * MapListProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MapListProcessor.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/GameSettings.h"
#include "../../lib/VCMI_Lib.h"

bool MapListProcessor::isMapSupported(const CMapInfo & info) const
{
	switch (info.mapHeader->version)
	{
		case EMapFormat::ROE:
			return VLC->settings()->getValue(EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA)["supported"].Bool();
		case EMapFormat::AB:
			return VLC->settings()->getValue(EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE)["supported"].Bool();
		case EMapFormat::SOD:
			return VLC->settings()->getValue(EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH)["supported"].Bool();
		case EMapFormat::WOG:
			return VLC->settings()->getValue(EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS)["supported"].Bool();
		case EMapFormat::HOTA:
			return VLC->settings()->getValue(EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS)["supported"].Bool();
		case EMapFormat::VCMI:
			return VLC->settings()->getValue(EGameSettings::MAP_FORMAT_JSON_VCMI)["supported"].Bool();
	}
	return false;
}

std::shared_ptr<CMapInfo> MapListProcessor::tryLoadMap(const std::string & mapName) const
{
	try
	{
		auto mapInfo = std::make_shared<CMapInfo>();
		mapInfo->mapInit(mapName);

		if (isMapSupported(*mapInfo))
			return mapInfo;
	}
	catch(std::exception & e)
	{
		logGlobal->error("Map %s is invalid. Message: %s", mapName, e.what());
	}
	return nullptr;
}

std::unordered_set<ResourcePath> MapListProcessor::getFiles(const std::string & dirURI, EResType resType) const
{
	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount)
	{
		return boost::algorithm::starts_with(mount, dirURI);
	});

	return CResourceHandler::get()->getFilteredFiles([&](const ResourcePath & ident)
	{
		return ident.getType() == resType && boost::algorithm::starts_with(ident.getName(), dirURI);
	});
}

std::vector<std::shared_ptr<CMapInfo>> MapListProcessor::getAllMaps() const
{
	std::unordered_set<ResourcePath> fileList = getFiles("MAPS/", EResType::MAP);
	std::vector<std::shared_ptr<CMapInfo>> result;

	result.reserve(fileList.size());

	for(auto & file : fileList)
	{
		auto mapInfo = tryLoadMap(file.getName());
		if (mapInfo)
			result.push_back(mapInfo);
	}

	return result;
}

std::vector<std::shared_ptr<CMapInfo>> MapListProcessor::getAllCampaigns() const
{
	std::unordered_set<ResourcePath> fileList = getFiles("MAPS/", EResType::CAMPAIGN);
	std::vector<std::shared_ptr<CMapInfo>> result;

	result.reserve(fileList.size());

	for(auto & file : fileList)
	{
		auto info = std::make_shared<CMapInfo>();
		//allItems[i].date = std::asctime(std::localtime(&files[i].date));
		info->fileURI = file.getName();
		info->campaignInit();
		if(info->campaign)
			result.push_back(info);
	}

	return result;
}

//void MapListProcessor::parseSaves(const std::unordered_set<ResourcePath> & files)
//{
//	for(auto & file : files)
//	{
//		try
//		{
//			auto mapInfo = std::make_shared<ElementInfo>();
//			mapInfo->saveInit(file);
//
//			// Filter out other game modes
//			bool isCampaign = mapInfo->scenarioOptionsOfSave->mode == EStartMode::CAMPAIGN;
//			bool isMultiplayer = mapInfo->amountOfHumanPlayersInSave > 1;
//			bool isTutorial = boost::to_upper_copy(mapInfo->scenarioOptionsOfSave->mapname) == "MAPS/TUTORIAL";
//			switch(CSH->getLoadMode())
//			{
//			case ELoadMode::SINGLE:
//				if(isMultiplayer || isCampaign || isTutorial)
//					mapInfo->mapHeader.reset();
//				break;
//			case ELoadMode::CAMPAIGN:
//				if(!isCampaign)
//					mapInfo->mapHeader.reset();
//				break;
//			case ELoadMode::TUTORIAL:
//				if(!isTutorial)
//					mapInfo->mapHeader.reset();
//				break;
//			default:
//				if(!isMultiplayer)
//					mapInfo->mapHeader.reset();
//				break;
//			}
//
//			allItems.push_back(mapInfo);
//		}
//		catch(const std::exception & e)
//		{
//			logGlobal->error("Error: Failed to process %s: %s", file.getName(), e.what());
//		}
//	}
//}
//
//void MapListProcessor::toggleMode()
//{
//	switch(tabType)
//	{
//		case ESelectionScreen::newGame:
//		{
//			inputName->disable();
//			auto files = getFiles("Maps/", EResType::MAP);
//			files.erase(ResourcePath("Maps/Tutorial.tut", EResType::MAP));
//			parseMaps(files);
//			break;
//		}
//
//		case ESelectionScreen::loadGame:
//			inputName->disable();
//			parseSaves(getFiles("Saves/", EResType::SAVEGAME));
//			break;
//
//		case ESelectionScreen::saveGame:
//			parseSaves(getFiles("Saves/", EResType::SAVEGAME));
//			inputName->enable();
//			inputName->activate();
//			restoreLastSelection();
//			break;
//
//		case ESelectionScreen::campaignList:
//			parseCampaigns(getFiles("Maps/", EResType::CAMPAIGN));
//			break;
//
//		default:
//			assert(0);
//			break;
//		}
//}
