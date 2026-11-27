--- Base of the battle action scripts of units. A derived script overrides the functions it needs
--- and calls the others through `self`, so that a script derived from it can override them too
local Script = {}
Script.__index = Script
Script.type = "battleAction"

--- True if the action offers the option to the unit on its turn
function Script:isAvailable(battle, unit, option)
	return true
end

--- Order in which the client tries the options for a hovered hex, lower first.
--- Nil uses the priority of the action config
function Script:getPriority(battle, option, unit, target)
	return nil
end

--- Cursor and status bar text for the option on the hovered hex
function Script:preview(battle, option, context)
	if not context.legal then
		return { cursor = "combatBlocked" }
	end
	return { cursor = "combatPointer" }
end

--- Destinations of the action made by clicking the hovered hex, or nil if the option has no action for the hex
function Script:buildTarget(battle, option, context)
	return nil
end

--- True if the side may make the action. Otherwise, explains the reason through `problem`
function Script:validate(battle, action, problem)
	return false
end

--- Applies the action through `env`
function Script:apply(env, battle, action)
end

return Script
