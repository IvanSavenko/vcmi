/*
 * BattleActionTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../IHandlerBase.h"

class BattleActionType;

/// Registry of battle action types, with the identifiers that name them. Loads the scripted types of the battleActions config section
class DLL_LINKAGE BattleActionTypeHandler final : public IHandlerBase
{
	std::vector<const BattleActionType *> typesById;
	/// in registration order, which is the order of collected options
	std::vector<const BattleActionType *> types;
	std::vector<std::string> jsonKeys;
	std::vector<std::shared_ptr<const BattleActionType>> scriptedTypes;

	void loadScriptedType(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data, size_t index);

public:
	explicit BattleActionTypeHandler(LibraryLoadContext & context);
	~BattleActionTypeHandler() override;

	std::vector<JsonNode> loadLegacyData(LibraryLoadContext & context) override;
	void loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data) override;
	void loadObject(LibraryLoadContext & context, const std::string & scope, const std::string & name, const JsonNode & data, size_t index) override;
	void afterLoadFinalization(LibraryLoadContext & context) override;

	/// Type with the id, or nullptr if no type has it
	const BattleActionType * getById(int32_t id) const;
	const std::vector<const BattleActionType *> & getAll() const;
	/// Full identifier of the type with the id. Throws if no type has it
	const std::string & getJsonKey(int32_t id) const;
};
