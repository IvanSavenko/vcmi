/*
 * ScenarioListEntry.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ScenarioListEntry.h"

#include "CMapHeader.h"
#include "CMapService.h"
#include "MapFormat.h"
#include "ScenarioMetadata.h"

#include "../GameLibrary.h"
#include "../GameSettings.h"
#include "../IGameSettings.h"
#include "../StartInfo.h"
#include "../campaign/CampaignHandler.h"
#include "../filesystem/Filesystem.h"
#include "../gameState/SaveMetadata.h"
#include "../modding/CModHandler.h"
#include "../modding/ModDescription.h"
#include "../serializer/CLegacyLoadFile.h"
#include "../serializer/CLoadFile.h"

ScenarioListEntry ScenarioListEntry::fromMap(const ResourcePath & file)
{
	CMapService mapService;
	const auto header = mapService.loadMapHeader(file);

	ScenarioListEntry result;
	result.type = EType::MAP;
	result.fileURI = file.getOriginalName();
	result.originalFileURI = file.getOriginalName();
	result.lastWrite = CResourceHandler::get()->getLastWriteTime(file);
	result.metadata["map"] = ScenarioMetadata::mapToJson(*header);
	result.metadata["mods"] = ScenarioMetadata::modsToJson(header->mods);
	return result;
}

ScenarioListEntry ScenarioListEntry::fromCampaign(const ResourcePath & file)
{
	const auto campaign = CampaignHandler::getHeader(file.getOriginalName());

	ScenarioListEntry result;
	result.type = EType::CAMPAIGN;
	result.fileURI = file.getOriginalName();
	result.originalFileURI = file.getOriginalName();
	result.lastWrite = CResourceHandler::get()->getLastWriteTime(file);
	result.metadata["campaign"] = ScenarioMetadata::campaignToJson(*campaign);
	// campaign maps are not parsed for the list, so mods required by them are unknown
	result.metadata["mods"].Struct();
	return result;
}

ScenarioListEntry ScenarioListEntry::fromSave(const ResourcePath & file, JsonValidator & validator)
{
	const auto saveFile = CResourceHandler::get()->getResourceName(file);
	if(!saveFile)
		throw std::runtime_error("Error: saved game '" + file.getOriginalName() + "' not found!");

	ScenarioListEntry result;
	result.type = EType::SAVE;
	result.fileURI = file.getName();
	result.originalFileURI = file.getOriginalName();
	result.lastWrite = CResourceHandler::get()->getLastWriteTime(file);

	if(CLoadFile::readVersion(*saveFile) < ESerializationVersion::SAVE_CHUNKS)
	{
		CMapHeader mapHeader;
		StartInfo startInfo;
		CLegacyLoadFile loader(*saveFile, nullptr);
		loader.loadHeader(mapHeader, startInfo);
		result.metadata = SaveMetadata::createLegacy(mapHeader, startInfo, loader.loadMods());
	}
	else
	{
		CLoadFile loader(*saveFile);
		result.metadata = SaveMetadata::parse(loader.readChunk(SaveFileFormat::CHUNK_METADATA), file.getOriginalName(), validator);
	}
	return result;
}

ScenarioListEntry::EStatus ScenarioListEntry::getStatus() const
{
	if(problem == EProblem::NONE)
		return EStatus::COMPATIBLE;
	if(problem != EProblem::MODS)
		return EStatus::INCOMPATIBLE;

	for(const auto & [modID, status] : mods)
		if(status != ModVerificationStatus::DISABLED && status != ModVerificationStatus::EXCESSIVE)
			return EStatus::INCOMPATIBLE;
	return EStatus::MODS_INACTIVE;
}

const JsonNode & ScenarioListEntry::getDescription() const
{
	return metadata[type == EType::CAMPAIGN ? "campaign" : "map"];
}

bool ScenarioListEntry::hasDescription() const
{
	return !getDescription().isNull();
}

std::string ScenarioListEntry::getName(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(getDescription()["name"], translator);
}

std::string ScenarioListEntry::getAuthor(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(getDescription()["author"], translator);
}

std::string ScenarioListEntry::getAuthorContact(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(getDescription()["authorContact"], translator);
}

std::string ScenarioListEntry::getVersion(const ITranslator * translator) const
{
	return ScenarioMetadata::textToString(getDescription()[type == EType::CAMPAIGN ? "campaignVersion" : "mapVersion"], translator);
}

std::time_t ScenarioListEntry::getCreationTime() const
{
	return getDescription()["creationDateTime"].Integer();
}

EMapFormat ScenarioListEntry::getMapFormat() const
{
	return static_cast<EMapFormat>(getDescription()["format"].Integer());
}

static std::optional<EGameSettings> getMapFormatOption(EMapFormat format)
{
	switch(format)
	{
		case EMapFormat::ROE:
			return EGameSettings::MAP_FORMAT_RESTORATION_OF_ERATHIA;
		case EMapFormat::AB:
			return EGameSettings::MAP_FORMAT_ARMAGEDDONS_BLADE;
		case EMapFormat::SOD:
			return EGameSettings::MAP_FORMAT_SHADOW_OF_DEATH;
		case EMapFormat::CHR:
			return EGameSettings::MAP_FORMAT_CHRONICLES;
		case EMapFormat::WOG:
			return EGameSettings::MAP_FORMAT_IN_THE_WAKE_OF_GODS;
		case EMapFormat::HOTA:
			return EGameSettings::MAP_FORMAT_HORN_OF_THE_ABYSS;
		case EMapFormat::VCMI:
			return EGameSettings::MAP_FORMAT_JSON_VCMI;
	}
	return std::nullopt;
}

const JsonNode & ScenarioListEntry::getMapFormatSettings() const
{
	const auto option = getMapFormatOption(getMapFormat());
	if(option)
		return LIBRARY->engineSettings()->getValue(*option);

	static const JsonNode unknownFormat;
	return unknownFormat;
}

std::vector<TModID> ScenarioListEntry::findMapFormatMods() const
{
	std::vector<TModID> result;
	const auto option = getMapFormatOption(getMapFormat());
	if(!option)
		return result;

	for(const auto & modID : LIBRARY->modh->getInstalledMods())
	{
		const JsonNode & modSettings = LIBRARY->modh->getModInfo(modID).getLocalValue("settings");
		if(GameSettings::findValue(modSettings, *option)["supported"].Bool())
			result.push_back(modID);
	}
	return result;
}

int ScenarioListEntry::getMapWidth() const
{
	return metadata["map"]["width"].Integer();
}

int ScenarioListEntry::getVictoryIcon() const
{
	return metadata["map"]["victoryIcon"].Integer();
}

int ScenarioListEntry::getDefeatIcon() const
{
	return metadata["map"]["defeatIcon"].Integer();
}

int ScenarioListEntry::getPlayers() const
{
	return metadata["map"]["players"].Integer();
}

int ScenarioListEntry::getHumanPlayers() const
{
	return metadata["map"]["humanPlayers"].Integer();
}

bool ScenarioListEntry::isCampaignSave() const
{
	return metadata["game"]["mode"].String() == "campaign";
}

bool ScenarioListEntry::isTutorialSave() const
{
	return metadata["game"]["mode"].String() == "tutorial";
}

int ScenarioListEntry::getHumanPlayersInSave() const
{
	return metadata["game"]["humanPlayers"].Integer();
}

std::string ScenarioListEntry::getMapFileInSave() const
{
	return metadata["game"]["mapFile"].String();
}

int ScenarioListEntry::getCampaignScenarioInSave() const
{
	return metadata["game"]["campaignScenario"].Integer();
}

int ScenarioListEntry::getCampaignScenarios() const
{
	return metadata["campaign"]["scenarios"].Integer();
}
