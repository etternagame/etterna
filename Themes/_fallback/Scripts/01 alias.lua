--- command aliases
-- @module 01_alias

--- @table _screen
-- shorthand! this is tedious to type and makes things ugly so let's make it shorter.
-- _screen.w, _screen.h, etc.
_screen = {
	w = SCREEN_WIDTH, -- screen width
	h = SCREEN_HEIGHT, -- screen height
	cx = SCREEN_CENTER_X, -- screen center x
	cy = SCREEN_CENTER_Y -- screen center y
}

function GetTitleSafeH(fPerc)
	return math.floor(SCREEN_RIGHT * fPerc)
end
function GetTitleSafeV(fPerc)
	return math.floor(SCREEN_BOTTOM * fPerc)
end

SAFE_WIDTH = GetTitleSafeH(0.075)
SAFE_HEIGHT = GetTitleSafeV(0.075)

--- Title & Action area safe calculation, probably messy.
-- Uses Microsoft's suggestion of 85% of the screen (7.5% per side).
-- @table _safe
_safe = {
	w = GetTitleSafeH(0.075), -- TitleSafeWidth
	h = GetTitleSafeV(0.075) -- TitleSafeHeight
}

--[[ compatibility aliases ]]
function alias_one(class, main_name, alt_name)
	if type(main_name) ~= "string" then
		lua.ReportScriptError("Name of function to make an alias for must be a string.")
		return
	end
	if type(alt_name) ~= "string" then
		lua.ReportScriptError("Alias name of function must be a string.")
		return
	end
	if class[alt_name] then
		return
	end
	class[alt_name] = class[main_name]
end

function alias_set(class, set)
	assert(
		type(class) == "table" and type(set) == "table",
		"alias_set must be passed a class and a set of names to make alieases."
	)
	for i, fun in ipairs(set) do
		if type(fun) == "table" then
			local main_name = fun[1]
			if type(set[2]) == "table" then
				for n, alt_name in ipairs(set[2]) do
					alias_one(class, main_name, alt_name)
				end
			elseif type(set[2]) == "string" then
				alias_one(class, main_name, set[2])
			end
		else
			lua.ReportScriptError("alias entry " .. i .. " in set passed to " .. "alias_set is not a table.")
		end
	end
end

function make_camel_aliases(class)
	local name_list = {}
	for name, fun in pairs(class) do
		if type(fun) == "function" and type(name) == "string" then
			name_list[#name_list + 1] = name
		end
	end
	for i, name in ipairs(name_list) do
		local words = split("_", name)
		for o, w in ipairs(words) do
			words[o] = w:sub(1, 1):upper() .. w:sub(2)
		end
		local camel_name = join("", words)
		if name ~= camel_name then
			alias_one(class, name, camel_name)
		end
	end
end

local to_camel_list = {
	CubicSplineN,
	NCSplineHandler,
	NoteColumnRenderer,
	NoteField
}

for i, class in ipairs(to_camel_list) do
	make_camel_aliases(class)
end

--[[ ActorScroller: all of these got renamed, so alias the lowercase ones if
themes are going to look for them. ]]
ActorScroller.getsecondtodestination = ActorScroller.GetSecondsToDestination
ActorScroller.setsecondsperitem = ActorScroller.SetSecondsPerItem
ActorScroller.setnumsubdivisions = ActorScroller.SetNumSubdivisions
ActorScroller.scrollthroughallitems = ActorScroller.ScrollThroughAllItems
ActorScroller.scrollwithpadding = ActorScroller.ScrollWithPadding
ActorScroller.setfastcatchup = ActorScroller.SetFastCatchup

--[[ MenuTimer: just some case changes. ]]
MenuTimer.setseconds = MenuTimer.SetSeconds

--- Aliases for old GAMESTATE timing functions. These have been converted to
-- SongPosition, but most themes still use these old functions.
function GameState:GetSongBeat()
	return self:GetSongPosition():GetSongBeat()
end
--- Aliases for old GAMESTATE timing functions. These have been converted to
-- SongPosition, but most themes still use these old functions.
function GameState:GetSongBeatNoOffset()
	return self:GetSongPosition():GetSongBeatNoOffset()
end
--- Aliases for old GAMESTATE timing functions. These have been converted to
-- SongPosition, but most themes still use these old functions.
function GameState:GetSongBPS()
	return self:GetSongPosition():GetCurBPS()
end
--- Aliases for old GAMESTATE timing functions. These have been converted to
-- SongPosition, but most themes still use these old functions.
function GameState:GetSongDelay()
	return self:GetSongPosition():GetDelay()
end
--- Aliases for old GAMESTATE timing functions. These have been converted to
-- SongPosition, but most themes still use these old functions.
function GameState:GetSongFreeze()
	return self:GetSongPosition():GetFreeze()
end

--- Conditional aliases for 3.9
-- @table Condition
Condition = {
	Hour = function()
		return Hour()
	end, -- Hour()
	IsDemonstration = function()
		return GAMESTATE:IsDemonstration()
	end, -- GAMESTATE:IsDemonstration()
	CurSong = function(sSongName)
		return GAMESTATE:GetCurrentSong():GetDisplayMainTitle() == sSongName
	end, -- GAMESTATE:GetCurrentSong():GetDisplayMainTitle() == sSongName
	DayOfMonth = function()
		return DayOfMonth()
	end, -- DayOfMonth()
	MonthOfYear = function()
		return MonthOfYear()
	end, -- MonthOfYear()
	UsingModifier = function(pnPlayer, sModifier)
		return GAMESTATE:PlayerIsUsingModifier(pnPlayer, sModifier)
	end -- GAMESTATE:PlayerIsUsingModifier( pnPlayer, sModifier )
}

--- Blend Modes
-- Aliases for blend modes.
-- @table Blend
Blend = {
	Normal = "BlendMode_Normal", -- BlendMode_Normal
	Add = "BlendMode_Add", -- BlendMode_Add
	Subtract = "BlendMode_Subtract", -- BlendMode_Subtract
	Modulate = "BlendMode_Modulate", -- BlendMode_Modulate
	CopySource = "BlendMode_CopySrc", -- BlendMode_CopySrc
	AlphaMask = "BlendMode_AlphaMask", -- BlendMode_AlphaMask
	AlphaKnockOut = "BlendMode_AlphaKnockOut", -- BlendMode_AlphaKnockOut
	AlphaMultiply = "BlendMode_AlphaMultiply", -- BlendMode_AlphaMultiply
	Multiply = "BlendMode_WeightedMultiply", -- BlendMode_WeightedMultiply
	Invert = "BlendMode_InvertDest", -- BlendMode_InvertDest
	NoEffect = "BlendMode_NoEffect" -- BlendMode_NoEffect
}
function StringToBlend(s)
	return Blend[s] or nil
end

--- EffectMode
-- Aliases for EffectMode (aka shaders)
-- @table EffectMode
EffectMode = {
	Normal = "EffectMode_Normal", -- EffectMode_Normal
	Unpremultiply = "EffectMode_Unpremultiply", -- EffectMode_Unpremultiply
	ColorBurn = "EffectMode_ColorBurn", -- EffectMode_ColorBurn
	ColorDodge = "EffectMode_ColorDodge", -- EffectMode_ColorDodge
	VividLight = "EffectMode_VividLight", -- EffectMode_VividLight
	HardMix = "EffectMode_HardMix", -- EffectMode_HardMix
	Overlay = "EffectMode_Overlay", -- EffectMode_Overlay
	Screen = "EffectMode_Screen", -- EffectMode_Screen
	YUYV422 = "EffectMode_YUYV422" -- EffectMode_YUYV422
}

--- Health Declarations
-- Used primarily for lifebars.
-- @table Health
Health = {
	Max = "HealthState_Hot", -- HealthState_Hot
	Alive = "HealthState_Alive", -- HealthState_Alive
	Danger = "HealthState_Danger", -- HealthState_Danger
	Dead = "HealthState_Dead" -- HealthState_Dead
}
