/*
 * api/Services.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Services.h"

#include <vcmi/Artifact.h>
#include <vcmi/Creature.h>
#include <vcmi/Faction.h>
#include <vcmi/HeroClass.h>
#include <vcmi/HeroType.h>
#include <vcmi/ResourceType.h>
#include <vcmi/Skill.h>
#include <vcmi/spells/Spell.h>

#include "../Registry.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"

#include "Artifact.h"
#include "Creature.h"
#include "Faction.h"
#include "HeroClass.h"
#include "HeroType.h"
#include "ResourceType.h"
#include "Skill.h"
#include "Spell.h"
#include "SpellSchool.h"

#include <vcmi/spells/SchoolService.h>


namespace scripting::api
{

static constexpr std::string_view SCOPE_DESCRIPTION = "Mod from which configuration the name was taken, see `scopeOf`. Defaults to the mod of the calling script.";

void ServicesProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&ServicesProxy::getArtifactByName>("getArtifactByName",
		{{"name", "JSON key of the artifact (e.g. `core:goldenBow`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up an artifact by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getCreatureByName>("getCreatureByName",
		{{"name", "JSON key of the creature (e.g. `core:pikeman`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a creature by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getFactionByName>("getFactionByName",
		{{"name", "JSON key of the faction (e.g. `core:castle`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a faction by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getHeroClassByName>("getHeroClassByName",
		{{"name", "JSON key of the hero class (e.g. `core:knight`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a hero class by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getHeroTypeByName>("getHeroTypeByName",
		{{"name", "JSON key of the hero type (e.g. `core:orrin`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a hero type by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getResourceByName>("getResourceByName",
		{{"name", "JSON key of the resource (e.g. `core:gold`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a resource by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getSpellByName>("getSpellByName",
		{{"name", "JSON key of the spell (e.g. `core:magicArrow`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a spell by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getSecondarySkillByName>("getSecondarySkillByName",
		{{"name", "JSON key of the secondary skill (e.g. `core:wisdom`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a secondary skill by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getSpellSchoolByName>("getSpellSchoolByName",
		{{"name", "JSON key of the spell school (e.g. `core:fire`)."}, {"scope", SCOPE_DESCRIPTION}}, {},
		"Looks up a spell school by its JSON key. Returns nil if not found.");
}

const Artifact * ServicesProxy::getArtifactByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->artifacts()->getByName(scope.value, name);
}

const Creature * ServicesProxy::getCreatureByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->creatures()->getByName(scope.value, name);
}

const Faction * ServicesProxy::getFactionByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->factions()->getByName(scope.value, name);
}

const HeroClass * ServicesProxy::getHeroClassByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->heroClasses()->getByName(scope.value, name);
}

const HeroType * ServicesProxy::getHeroTypeByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->heroTypes()->getByName(scope.value, name);
}

const ResourceType * ServicesProxy::getResourceByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->resources()->getByName(scope.value, name);
}

const spells::Spell * ServicesProxy::getSpellByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->spells()->getByName(scope.value, name);
}

const Skill * ServicesProxy::getSecondarySkillByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->skills()->getByName(scope.value, name);
}

const spells::SpellSchoolType * ServicesProxy::getSpellSchoolByName(const Services * services, const std::string & name, const ScriptModScope & scope)
{
	return services->spellSchools()->getByName(scope.value, name);
}

}
