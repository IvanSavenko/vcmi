#pragma once

#include "ObjectTemplate.h"

#include "../GameConstants.h"
#include "../ConstTransitivePtr.h"
#include "../IHandlerBase.h"

/*
 * CObjectClassesHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class JsonNode;
class CRandomGenerator;

class IObjectInfo
{
public:
	virtual bool givesResources() const = 0;

	virtual bool givesExperience() const = 0;
	virtual bool givesMana() const = 0;
	virtual bool givesMovement() const = 0;

	virtual bool givesPrimarySkills() const = 0;
	virtual bool givesSecondarySkills() const = 0;

	virtual bool givesArtifacts() const = 0;
	virtual bool givesCreatures() const = 0;
	virtual bool givesSpells() const = 0;

	virtual bool givesBonuses() const = 0;
};

class CGObjectInstance;

class AObjectTypeHandler
{
	si32 type;
	si32 subtype;

	JsonNode base; /// describes base template

	std::vector<ObjectTemplate> templates;
protected:

	virtual bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;
public:
	virtual ~AObjectTypeHandler(){}

	void setType(si32 type, si32 subtype);

	/// loads templates from Json structure using fields "base" and "templates"
	virtual void init(const JsonNode & input);

	void addTemplate(ObjectTemplate templ);
	void addTemplate(JsonNode config);

	/// returns all templates, without any filters
	std::vector<ObjectTemplate> getTemplates() const;

	/// returns all templates that can be placed on specific terrain type
	std::vector<ObjectTemplate> getTemplates(si32 terrainType) const;

	/// returns preferred template for this object, if present (e.g. one of 3 possible templates for town - village, fort and castle)
	/// note that appearance will not be changed - this must be done separately (either by assignment or via pack from server)
	boost::optional<ObjectTemplate> getOverride(si32 terrainType, const CGObjectInstance * object) const;

	/// Creates object and set up core properties (like ID/subID). Object is NOT initialized
	/// to allow creating objects before game start (e.g. map loading)
	virtual CGObjectInstance * create(ObjectTemplate tmpl) const = 0;

	/// Configures object properties. Should be re-entrable, resetting state of the object if necessarily
	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const = 0;

	/// Returns object configuration, if available. Othervice returns NULL
	virtual const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & subtype & templates;
	}
};

/// Class that is used for objects that do not have dedicated handler
template<class ObjectType>
class CDefaultObjectTypeHandler : public AObjectTypeHandler
{
	CGObjectInstance * create(ObjectTemplate tmpl) const
	{
		auto obj = new ObjectType();
		obj->ID = tmpl.id;
		obj->subID = tmpl.subid;
		obj->appearance = tmpl;
		return obj;
	}

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
	{
	}

	virtual const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const
	{
		return nullptr;
	}
};

typedef std::shared_ptr<AObjectTypeHandler> TObjectTypeHandler;

class DLL_LINKAGE CObjectClassesHandler : public IHandlerBase
{
	/// Small internal structure that contains information on specific group of objects
	/// (creating separate entity is overcomplicating at least at this point)
	struct ObjectContainter
	{
		si32 id;

		std::string name; // human-readable name
		std::string handlerName; // ID of handler that controls this object, shoul be determined using hadlerConstructor map

		JsonNode base;
		std::map<si32, TObjectTypeHandler> objects;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & base & objects;
		}
	};

	typedef std::multimap<std::pair<si32, si32>, ObjectTemplate> TTemplatesContainer;

	/// list of object handlers, each of them handles only one type
	std::map<si32, ObjectContainter * > objects;

	/// map that is filled during contruction with all known handlers. Not serializeable
	std::map<std::string, std::function<TObjectTypeHandler()> > handlerConstructors;

	/// container with H3 templates, used only during loading
	TTemplatesContainer legacyTemplates;

	void loadObjectEntry(const JsonNode & entry, ObjectContainter * obj);
	ObjectContainter * loadFromJson(const JsonNode & json);
public:
	CObjectClassesHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void createObject(std::string name, JsonNode config, si32 ID, boost::optional<si32> subID = boost::optional<si32>());
	void eraseObject(si32 ID, si32 subID);

	void beforeValidate(JsonNode & object) override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;

	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	TObjectTypeHandler getHandlerFor(si32 type, si32 subtype) const;

	std::string getObjectName(si32 type) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}
};