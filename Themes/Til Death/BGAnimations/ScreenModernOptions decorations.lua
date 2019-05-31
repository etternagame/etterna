local function p(str)
	MESSAGEMAN:Broadcast("ScriptError", {message = tostring(str)})
end
--[[
	Option Utility module
	types of options:
	"keymapping": The option name label is the button name, and there's a clickable label with the
		current key, that when clicked expects the new key for the button
	"button": The option name label is a clickable button that executes onClick
	"list": Lists all the options (In the possibleValues table of strings) as clickable labels
	"largeList": Like list but doesnt display all options, displays current has next/prev buttons
	"range": Displays current value (clamped in range.min and range.max). Can be a slider
	"rangeWithSteps": Displays current value and has a button for each step in steps (table of numbers)

	option:
	{
		name = string,
		-- These 2 are only nil for button options
		setter = function(string) or nil,
		getter = string function() or nil,
		-- output value, or a list of selections
		type = "keymapping" or "button" or "list" or "largeList" or "range" or "rangeWithSteps",
		-- not nil for list or largeList
		possibleValues = nil or {string},
		-- not nil for range type (step defaults to 1)
		range = nil or {min=number,max=number, step=number or nil},
		-- not nil for rangeWithSteps
		steps = {number},
		-- not nil for keymapping
		button = string,
		onClick = nil or function()
	}
	section: {name = string, {option}}
	tab: {nameString, {section}}
--]]
OptionUtils = {}
-- "constructors"
local function optionLineData(name, setter, getter, type, values)
	return {
		name = name,
		setter = setter,
		getter = getter,
		-- output value, or a list of selections
		type = type,
		possibleValues = values
	}
end
OptionUtils.optionLineData = optionLineData
local function listOptionLineData(name, setter, getter, values)
	return {
		name = name,
		setter = setter,
		getter = getter,
		-- output value, or a list of selections
		type = "list",
		possibleValues = values
	}
end
OptionUtils.listOptionLineData = listOptionLineData
local function largeListOptionLineData(name, setter, getter, values)
	return {
		name = name,
		setter = setter,
		getter = getter,
		-- output value, or a list of selections
		type = "largeList",
		possibleValues = values
	}
end
OptionUtils.largeListOptionLineData = largeListOptionLineData
local function rangeOptionLineData(name, setter, getter, min, max, step)
	return {
		name = name,
		setter = setter,
		getter = getter,
		-- output value, or a list of selections
		type = "range",
		range = {min = min, max = max, step = step or 1}
	}
end
OptionUtils.rangeOptionLineData = rangeOptionLineData
local nilFunction = function()
end
OptionUtils.nilFunction = nilFunction
local function buttonOptionLineData(name, onClick)
	return {
		name = name,
		-- output value, or a list of selections
		type = "button",
		onClick = onClick,
		setter = nilFunction,
		getter = nilFunction
	}
end
OptionUtils.buttonOptionLineData = buttonOptionLineData
local function keymappingOptionLineData(button)
	return {
		name = button,
		button = button,
		setter = nilFunction,
		getter = nilFunction,
		-- output value, or a list of selections
		type = "keymapping"
	}
end
OptionUtils.keymappingOptionLineData = keymappingOptionLineData
--[[
	Wrapper around listOptionLineData that lets your setter and getter handle
	bools instead of strings
]]
local function booleanOptionLineData(name, setter, getter)
	return listOptionLineData(
		name,
		function(str)
			setter(str == "On")
		end,
		function()
			return getter() and "On" or "Off"
		end,
		{"On", "Off"}
	)
end
OptionUtils.booleanOptionLineData = booleanOptionLineData
--[[
	Returns a setter function for the preference with the given name.
	if mapper is nil it sets to the given parameter directly, if its a function it
	gives it the parameter to map into preference values, and if its a table its indexed
	the hook function is called after setting with the pref value and mapped value
]]
local function prefSetter(name, mapper, hook)
	return mapper and (type(mapper) == "function" and function(str)
				local mapped = mapper(str)
				PREFSMAN:SetPreference(name, mapped)
				if hook then
					hook(str, mapped)
				end
			end or function(str)
				local mapped = mapper[str]
				PREFSMAN:SetPreference(name, mapped)
				if hook then
					hook(str, mapped)
				end
			end) or function(str)
			PREFSMAN:SetPreference(name, str)
			if hook then
				hook(str, str)
			end
		end
end
OptionUtils.prefSetter = prefSetter
--[[
	Returns a getter function for the preference with the given name.
	if mapper is nil it returns the preference value directly, if its a function it
	gives it the pref vaule to map into return value, and if its a table its indexed
]]
local function prefGetter(name, mapper)
	return mapper and (type(mapper) == "function" and function()
				return mapper(PREFSMAN:GetPreference(name))
			end or function()
				return mapper[PREFSMAN:GetPreference(name)]
			end) or function()
			return PREFSMAN:GetPreference(name)
		end
end
OptionUtils.prefGetter = prefGetter
local function bitnessPrefOptionLineData(name, pref)
	return listOptionLineData(
		name,
		prefSetter(
			pref,
			function(str)
				return tonumber(str:match("(%d+)bit"))
			end
		),
		prefGetter(
			pref,
			function(bits)
				return tostring(bits) .. "bit"
			end
		),
		{"16bit", "32bit"}
	)
end
OptionUtils.bitnessPrefOptionLineData = bitnessPrefOptionLineData
local function booleanPrefOptionLineData(name, prefName)
	return booleanOptionLineData(name, prefSetter(prefName), prefGetter(prefName))
end
OptionUtils.booleanPrefOptionLineData = booleanPrefOptionLineData
-- error checking for option lines
--TODO: finish checkers
local optionLineRequirements = {
	global = function(optLine)
		if not optLine.name or type(optLine.name) ~= "string" then
			error "A line option requires a name string"
		end
		if not optLine.setter or type(optLine.setter) ~= "function" then
			error "A line option requires a setter function"
		end
		if not optLine.getter or type(optLine.getter) ~= "function" then
			error "A line option requires a getter function"
		end
		if not optLine.type or type(optLine.type) ~= "string" then
			error "A line option requires a type string"
		end
	end,
	keymapping = function(optLine)
	end,
	button = function(optLine)
		if not optLine.onClick or type(optLine.onClick) ~= "function" then
			error "A button option requires an onClick function"
		end
	end,
	list = function(optLine)
		if not optLine.possibleValues or type(optLine.possibleValues) ~= "table" or #(optLine.possibleValues) == 0 then
			error "A list option requires a possibleValues table with at least 1 string"
		end
	end,
	largeList = function(optLine)
		if not optLine.possibleValues or type(optLine.possibleValues) ~= "table" or #(optLine.possibleValues) == 0 then
			error "A largeList option requires a possibleValues table with at least 1 string"
		end
	end,
	range = function(optLine)
		if not optLine.range or type(optLine.range) ~= "table" then
			error "A range option requires a range table"
		end
		if not tonumber(optLine.range.max) or not tonumber(optLine.range.min) then
			error "A range option requires the range table to have a min and a max"
		end
	end,
	rangeWithSteps = function(optLine)
		if not optLine.steps or type(optLine.steps) ~= "table" or #(optLine.steps) == 0 then
			error "A range option requires a steps table with at least 1 number"
		end
	end
}
OptionUtils.optionLineRequirements = optionLineRequirements
local function defaultOptionLineBuilders(font, lineHeight, nameLabelWidth)
	font.halign = 0
	return {
		keymapping = function(optionLineData)
			local expectingInput = false
			local opt = Widg.Container {}
			local highlight = function(button)
				button.label.actor:diffusealpha(1)
			end
			local unhighlight = function(button)
				button.label.actor:diffusealpha(0.7)
			end
			local buttonContainer = Widg.Container {x = nameLabelWidth}
			keyButton =
				Widg.Button {
				onUnhighlight = unhighlight,
				onHighlight = highlight,
				text = optionLineData.getter(),
				height = lineHeight,
				font = font,
				halign = 0.5,
				alpha = 0,
				width = nameLabelWidth,
				onInit = function(button)
					button.label.actor:addx(-nameLabelWidth / 2)
				end,
				onClick = function()
					-- TODO: handle input and decide on a format for key values
					SCREENMAN:set_input_redirected(PLAYER_1, true)
					expectingInput = true
				end
			}
			opt.name =
				Widg.Label(setmetatable({text = optionLineData.name, width = nameWidth, y = lineHeight / 2}, {__index = font}))
			opt:add(opt.name)
			buttonContainer:add(keyButton)
			opt:add(buttonContainer)
			local keymapper = function(input)
				if not expectingInput then
					return
				end
				local isRelease = input.type == "InputEventType_Release"
				if isRelease and input.DeviceInput.button == "DeviceButton_escape" then
					SCREENMAN:GetTopScreen():setTimeout(
						function()
							SCREENMAN:set_input_redirected(PLAYER_1, false)
						end,
						0.0001
					)
					return false
				end
				-- TODO: validate that its a mappable character
				-- Should we expose a c++ function for it?
				if isRelease or false then
					-- TODO: Expose INPUTMAPPER:SetInputMap(m_DeviceI, gameInputString)
					--INPUTMAPPER:SetInputMap(input, "1_"..optionLineData.button);
					SCREENMAN:set_input_redirected(PLAYER_1, false)
					return false
				end
			end
			opt.BeginCommand = function()
				SCREENMAN:GetTopScreen():AddInputCallback(keymapper)
			end
			return opt
		end,
		button = function(optionLineData)
			local highlight = function(button)
				button.actor:diffusealpha(1)
			end
			local unhighlight = function(button)
				button.actor:diffusealpha(0.7)
			end
			return Widg.Button {
				onUnhighlight = unhighlight,
				onHighlight = highlight,
				text = optionLineData.name,
				height = lineHeight,
				y = -lineHeight / 2,
				x = nameLabelWidth / 2,
				font = font,
				halign = 1,
				valign = 0,
				alpha = 0,
				width = nameLabelWidth,
				onInit = function(button)
					button.label.actor:halign(0):x(0)
				end,
				onClick = optionLineData.onClick
			}
		end,
		list = function(optionLineData)
			local opt
			opt =
				Widg.Container {
				x = nameLabelWidth,
				onInit = function()
					local current = optionLineData.getter()
					for k, v in pairs(opt.labels) do
						v.label.actor:diffusealpha(k ~= current and 0.7 or 1.0)
					end
				end
			}
			opt.labels = {}
			local highlight = function(button)
				button.label.actor:diffusealpha(1)
			end
			local unhighlight = function(button)
				button.label.actor:diffusealpha(button.selected and 1.0 or 0.7)
			end
			local initialValue = optionLineData.getter()
			for i, v in ipairs(optionLineData.possibleValues) do
				-- TODO: Make x spacing use actual label width
				local label =
					Widg.Button {
					x = 0,
					onUnhighlight = unhighlight,
					onHighlight = highlight,
					text = v,
					font = font,
					width = v:len() * 8,
					height = lineHeight,
					halign = 0.5,
					alpha = 0,
					onClick = function(button)
						optionLineData.setter(v)
						for k, v in pairs(opt.labels) do
							v.selected = false
						end
						button.selected = true
					end
				}
				opt.labels[v] = label
				opt:add(label)
				if initialValue == v then
					label.selected = true
				end
			end
			opt.name =
				Widg.Label(
				setmetatable({halign = 0, text = optionLineData.name, x = -nameLabelWidth, y = lineHeight / 2}, {__index = font})
			)
			opt:add(opt.name)
			local spacingX = 15
			opt.BeginCommand = function()
				-- position using widths
				local acumX = 0
				for i, v in ipairs(optionLineData.possibleValues) do
					label = opt.labels[v]
					label.actor:x(acumX)
					label.label.actor:x(0)
					acumX = label.label.actor:GetZoomedWidth() + acumX + spacingX
				end
			end
			return opt
		end,
		largeList = function(optionLineData, nameWidth)
			local opt = Widg.Container {}
			local optSelector =
				Widg.Container {
				x = nameLabelWidth
			}
			local function update(str)
				optSelector.label.label.actor:maxwidth(1000)
				optSelector.label:settext(str)
				local selectionWidth = optSelector.label.label.actor:GetZoomedWidth()

				optSelector.label.bg.actor:zoomto(selectionWidth, optSelector.label.bg.actor:GetZoomedHeight())
				optSelector.label.actor:x((selectionWidth / 2) - 5)
				optSelector.nextButton.actor:x(12 + selectionWidth)
				optSelector.prevButton.label.actor:x(0)
			end
			optSelector.BeginCommand = function()
				local current = optionLineData.getter()
				update(current)
			end
			local indexes = {}
			local possibleValues = optionLineData.possibleValues
			local valueCount = #possibleValues
			for i, v in ipairs(possibleValues) do
				indexes[v] = i
			end
			local highlight = function(button)
				button.label.actor:diffusealpha(1)
			end
			local unhighlight = function(button)
				button.label.actor:diffusealpha(0.7)
			end
			optSelector.prevButton =
				Widg.Button {
				onUnhighlight = unhighlight,
				onHighlight = highlight,
				text = "<<",
				font = font,
				alpha = 0,
				height = lineHeight,
				width = 25,
				onClick = function(button)
					local curIdx = indexes[optSelector.label.label:GetText()]
					local newIdx = ((curIdx - 1 - 1) % valueCount) + 1
					local newVal = possibleValues[newIdx]
					optionLineData.setter(newVal)
					update(newVal)
				end
			}
			optSelector.nextButton =
				Widg.Button {
				onUnhighlight = unhighlight,
				onHighlight = highlight,
				text = ">>",
				alpha = 0,
				font = font,
				height = lineHeight,
				width = 25,
				onClick = function(button)
					local curIdx = indexes[optSelector.label.label:GetText()]
					local newIdx = ((curIdx - 1 + 1) % valueCount) + 1
					local newVal = possibleValues[newIdx]
					optionLineData.setter(newVal)
					update(newVal)
				end
			}
			-- todo: dynamic x here
			optSelector.label =
				Widg.Button {
				font = setmetatable({halign = 0.5}, {__index = font}),
				alpha = 0
			}
			optSelector:add(optSelector.prevButton)
			optSelector:add(optSelector.nextButton)
			optSelector:add(optSelector.label)
			opt.name =
				Widg.Label(setmetatable({text = optionLineData.name, width = nameWidth, y = lineHeight / 2}, {__index = font}))
			opt:add(opt.name)
			opt:add(optSelector)
			return opt
		end,
		range = function(optionLineData)
			local sliderWidth = 150
			local arrowWidth = 20
			local opt
			opt =
				Widg.Container {
				x = nameLabelWidth
			}
			opt.name =
				Widg.Label(
				setmetatable({halign = 0, text = optionLineData.name, x = -nameLabelWidth, y = lineHeight / 2}, {__index = font})
			)
			local initialValue = optionLineData.getter()
			opt.currentValueLabel =
				Widg.Label(
				setmetatable(
					{halign = 0, text = tostring(initialValue), x = 1.05 * sliderWidth + arrowWidth * 2, y = lineHeight / 2},
					{__index = font}
				)
			)
			opt:add(opt.name)
			opt.slider =
				Widg.SliderBase {
				width = sliderWidth,
				height = lineHeight,
				y = lineHeight / 2,
				x = sliderWidth / 2 + arrowWidth,
				defaultValue = initialValue,
				min = optionLineData.range.min,
				visible = true,
				handle = function(params)
					local h
					h =
						Widg.Rectangle {
						color = getTitleColor("Line_Left"),
						width = params.width / 15,
						height = params.height,
						valign = params.valign
					}
					h.onValueChange = function(val)
						h.actor:x(val * params.width / (params.max - params.min) - params.width * params.halign * 0.5)
					end
					return h
				end,
				bar = function(params)
					return Widg.Rectangle {
						color = getTitleColor("Line_Right"),
						width = params.width,
						height = params.height / 15,
						halign = params.halign,
						valign = params.valign
					}
				end,
				max = optionLineData.range.max,
				onValueChange = function(val)
					opt.currentValueLabel.actor:settext(tostring(val))
					optionLineData.setter(val)
				end
			}
			opt.prevButton =
				Widg.Button {
				onUnhighlight = unhighlight,
				onHighlight = highlight,
				text = "<<",
				font = font,
				alpha = 0,
				height = lineHeight,
				width = arrowWidth * 5 / 6,
				halign = 1,
				onClick = function(button)
					local v = math.max(opt.slider.value - optionLineData.range.step, optionLineData.range.min)
					opt.slider:SetValue(v)
				end
			}
			opt.nextButton =
				Widg.Button {
				onUnhighlight = unhighlight,
				onHighlight = highlight,
				text = ">>",
				alpha = 0,
				font = font,
				height = lineHeight,
				x = 1.05 * sliderWidth + arrowWidth,
				width = arrowWidth * 5 / 6,
				halign = 1,
				onClick = function(button)
					local v = math.min(opt.slider.value + optionLineData.range.step, optionLineData.range.max)
					opt.slider:SetValue(v)
				end
			}
			opt:add(opt.slider)
			opt:add(opt.currentValueLabel)
			opt:add(opt.prevButton)
			opt:add(opt.nextButton)
			return opt
		end,
		rangeWithSteps = function(optLine)
			--TODO: rangeWithSteps line maker
		end
	}
end
OptionUtils.defaultOptionLineBuilders = defaultOptionLineBuilders
local function makeOptionLine(optLine, builders)
	optionLineRequirements.global(optLine)
	local type = optLine.type
	optionLineRequirements[type](optLine)
	return builders[type](optLine)
end
OptionUtils.makeOptionLine = makeOptionLine

local function getPlayerOptions()
	return GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptions("ModsLevel_Preferred")
end
OptionUtils.getPlayerOptions = getPlayerOptions
local function getScrollSpeed()
	local optObj = getPlayerOptions()
	--MMod must be before XMod because if MMod is active XMod also returns true
	--Handles conversion of XMod to 100's
	--SCREENMAN:SystemMessage("mmod: "..tostring(optObj:MMod()).. " xmod: "..tostring(optObj:XMod()).." cmod: " ..tostring(optObj:CMod()))
	return (optObj:MMod() or optObj:CMod() or optObj:XMod() and optObj:XMod() * 100)
end
OptionUtils.getScrollSpeed = getScrollSpeed
local function setScrollSpeed(sp)
	local optObj = getPlayerOptions()
	if optObj:MMod() then
		optObj:MMod(sp, 1)
	elseif optObj:CMod() then
		optObj:CMod(sp, 1)
	elseif optObj:XMod() then
		optObj:XMod(sp / 100, 1)
	end
end
OptionUtils.setScrollSpeed = setScrollSpeed
local function reverse(t)
	local reversed = {}
	for i, v in ipairs(t) do
		reversed[v] = i
	end
	for k, v in pairs(t) do
		reversed[v] = k
	end
	return reversed
end
OptionUtils.reverse = reverse
local function ScreenChange(name)
	return function()
		if SCREENMAN and SCREENMAN:GetTopScreen() and SCREENMAN:GetTopScreen():GetName() ~= name then
			SCREENMAN:GetTopScreen():SetNextScreenName(name):StartTransitioningScreen("SM_GoToNextScreen")
		end
	end
end
OptionUtils.ScreenChange = ScreenChange
OptionUtils.Options = {
	Language = largeListOptionLineData(
		"Language",
		function(str)
		end,
		function()
			return "t"
			--return THEME:GetCurLanguage()
		end,
		-- THEME:GetLanguages(), -- TODO: Add this in C++
		{"Englasdasdassdasdish", "t"}
	),
	Gametype = listOptionLineData(
		"Gametype",
		function(str)
			GAMEMAN:SetGame(keyCountToGameName[str])
			if str == "6k" then
				-- todo: Why is it not setting solo?
				GAMESTATE:SetCurrentStyle("solo")
			elseif str == "4k" then
				GAMESTATE:SetCurrentStyle("single")
			end
		end,
		function()
			local gameName = GAMESTATE:GetCurrentGame():GetName()
			if gameName == "dance" then
				do
					return "4k"
				end
				-- TODO: fix this
				local style = GAMESTATE:GetCurrentStyle():GetName()
				if style == "single" then
					return "4k"
				else
					return "6k"
				end
			end
			return gameNameToKeyCount[GAMESTATE:GetCurrentGame():GetName()]
		end,
		{"4k", "5k", "6k", "7k"}
	),
	Judge = listOptionLineData(
		"Judge",
		function(str)
			SetTimingDifficulty(judgesToWindow[tonumber(str)])
		end,
		function()
			return tostring(GetTimingDifficulty())
		end,
		{"1", "2", "3", "4", "5", "6", "7", "8", "9"}
	),
	ScrollDirection = listOptionLineData(
		"Scroll direction",
		function(str)
			getPlayerOptions():Reverse(str == "Downscroll")
		end,
		function()
			return getPlayerOptions():UsingReverse() and "Downscroll" or "Upscroll"
		end,
		{"Upscroll", "Downscroll"}
	),
	ScrollType = listOptionLineData(
		"Scroll type",
		function(str)
			local options = getPlayerOptions()
			options[str](options, getScrollSpeed(), 1)
		end,
		function()
			local options = getPlayerOptions()
			return (options:XMod() and "XMod") or (options:CMod() and "CMod") or (options:MMod() and "MMod")
		end,
		{"XMod", "CMod"}
	),
	CalibrateAudioSync = buttonOptionLineData("Calibrate Audio Sync", ScreenChange("ScreenGameplaySyncMachine")),
	CenterReceptors = booleanPrefOptionLineData("Center receptors", "Center1Player"),
	Theme = largeListOptionLineData(
		"Theme",
		function(str)
			THEME:SetTheme(str)
		end,
		function()
			return THEME:GetCurThemeName()
		end,
		THEME:GetSelectableThemeNames()
	),
	DisplayMode = largeListOptionLineData(
		"Display mode",
		prefSetter(
			"Windowed",
			{["Windowed"] = true, ["Fullscreen"] = false},
			function()
				THEME:SetTheme(THEME:GetCurThemeName())
			end
		),
		prefGetter("Windowed", {[true] = "Windowed", [false] = "Fullscreen"}),
		{"Windowed", "Fullscreen"}
	),
	AspectRatio = largeListOptionLineData(
		"Aspect ratio",
		prefSetter(
			"DisplayAspectRatio",
			function(string)
				local dividend, divider = string:match("(%d+):(%d+)")
				return dividend / divider
			end
		),
		prefGetter(
			"DisplayAspectRatio",
			function(ratio)
				local values = {
					[3 / 4] = "3:4",
					[1] = "1:1",
					[4 / 3] = "4:3",
					[5 / 4] = "5:4",
					[16 / 9] = "16:9",
					[16 / 10] = "16:10",
					[8 / 3] = "8:3"
				}
				for r, name in pairs(values) do
					if math.abs(ratio - r) < 0.001 then
						return name
					end
				end
			end
		),
		{"3:4", "1:1", "4:3", "5:4", "16:9", "16:10", "8:3"}
	),
	Resolution = largeListOptionLineData(
		"Display resolution",
		function(str)
			--TODO
		end,
		function()
			return "800x600"
		end,
		{"800x600"}
	),
	OverscanCorrection = buttonOptionLineData("Overscan correction", ScreenChange("ScreenOverscanConfig")),
	DisplayColorDepth = bitnessPrefOptionLineData("Display color", "DisplayColorDepth"),
	TextureColorDepth = bitnessPrefOptionLineData("Texture color", "TextureColorDepth"),
	MovieColorDepth = bitnessPrefOptionLineData("Movie color", "MovieColorDepth"),
	MaxTextureResolution = largeListOptionLineData(
		"Texture resolution",
		prefSetter("MaxTextureResolution", tonumber),
		prefGetter("MaxTextureResolution", tostring),
		{
			"256",
			"512",
			"1024",
			"2048"
		}
	),
	HighResolutionTextures = largeListOptionLineData(
		"High-res textures",
		prefSetter(
			"HighResolutionTextures",
			function(str)
				return str:gsub(" ", "")
			end
		),
		prefGetter(
			"HighResolutionTextures",
			function(str)
				return str:match("HighResolutionTextures_(%a+)"):gsub("eO", "e O")
			end
		),
		{"Auto", "Force On", "Force Off"}
	),
	RefreshRate = largeListOptionLineData(
		"Refresh rate",
		prefSetter(
			"RefreshRate",
			function(rate)
				return rate == "Default" and 0 or tonumber(rate)
			end
		),
		prefGetter(
			"RefreshRate",
			function(rate)
				return rate == 0 and "Default" or tostring(rate)
			end
		),
		{
			"Default",
			"60",
			"70",
			"72",
			"75",
			"80",
			"85",
			"90",
			"100",
			"120",
			"150"
		}
	),
	Vsync = booleanPrefOptionLineData("Wait for VSync", "Vsync")
}
-- utility module ends here
local function tab(name, sections)
	return {
		name = name,
		sections = sections
	}
end
local function section(name, lines)
	if lines.name then
		lines = {lines}
	end
	lines.name = name
	return lines
end
local reverse = OptionUtils.reverse
local defaultOptionLineBuilders = OptionUtils.defaultOptionLineBuilders
local makeOptionLine = OptionUtils.makeOptionLine
local buttonOptionLineData = OptionUtils.buttonOptionLineData
local keymappingOptionLineData = OptionUtils.keymappingOptionLineData
local booleanOptionLineData = OptionUtils.booleanOptionLineData

-- TODO: move some of these
-- TODO: profile selector
local profileIDs = PROFILEMAN:GetLocalProfileIDs()
local profileNames = {}
for i, v in ipairs(profileIDs) do
	local pro = PROFILEMAN:GetLocalProfileFromIndex(PROFILEMAN:GetLocalProfileIndexFromID(v))
	profileNames[i] = pro:GetDisplayName()
end
local curProfileID = profileIDs[1]
local curProfileDir = "Save/LocalProfiles/" .. curProfileID .. "/"
playerConfig:load(pn_to_profile_slot(PLAYER_1), curProfileDir)
local DisplayPercent =
	booleanOptionLineData(
	"Current Percent",
	function(bool)
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).DisplayPercent = bool
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1), curProfileDir)
	end,
	function()
		return playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).DisplayPercent
	end,
	1,
	100
)
local JudgeCounter =
	booleanOptionLineData(
	"Judge Counter",
	function(bool)
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter = bool
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1), curProfileDir)
	end,
	function()
		return playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter
	end
)
local JudgmentText =
	booleanOptionLineData(
	"JudgmentText",
	function(bool)
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgmentText = bool
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1), curProfileDir)
	end,
	function()
		return playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgmentText
	end
)
local PlayerInfo =
	booleanOptionLineData(
	"PlayerInfo",
	function(bool)
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PlayerInfo = bool
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1), curProfileDir)
	end,
	function()
		return playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PlayerInfo
	end
)
local BGBrightness = rangeOptionLineData("BGBrightness", prefSetter("BGBrightness"), prefGetter("BGBrightness"), 0, 100)
local MeasureLines =
	booleanOptionLineData(
	"Measure lines",
	function(bool)
		themeConfig:get_data().global.MeasureLines = bool
		themeConfig:set_dirty()
		themeConfig:save()
		THEME:ReloadMetrics()
	end,
	function()
		return themeConfig:get_data().global.MeasureLines
	end
)
local ReceptorSize =
	rangeOptionLineData(
	"Receptor Size",
	function(n)
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ReceptorSize = n
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1), curProfileDir)
	end,
	function()
		return playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ReceptorSize
	end,
	1,
	200
)
local ScreenFilter =
	rangeOptionLineData(
	"ScreenFilter",
	function(n)
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ScreenFilter = n
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1), curProfileDir)
	end,
	function()
		return playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ScreenFilter
	end,
	0,
	100
)

local tabs
do
	local judgesToWindow = {1.50, 1.33, 1.16, 1.00, 0.84, 0.66, 0.50, 0.33, 0.20}
	local keyCountToGameName = {
		["5k"] = "pump",
		["7k"] = "KB7",
		["6k"] = "dance",
		["4k"] = "dance"
	}
	-- TODO: Put the non til death only options (All of them or almost all)
	-- in some utility table in the options module
	local gameNameToKeyCount = reverse(keyCountToGameName)
	local optionLines = OptionUtils.Options
	local gameplay =
		tab(
		"Gameplay",
		{
			section(
				"Essential Options",
				{
					optionLines.Language,
					optionLines.Gametype,
					optionLines.Judge,
					optionLines.ScrollDirection,
					optionLines.ScrollType,
					--todo: speed
					--todo:noteskin
					optionLines.CalibrateAudioSync,
					buttonOptionLineData(
						"Customize Gameplay",
						function()
							p "click"
						end
					)
				}
			),
			section(
				"Visuals",
				{
					optionLines.CenterReceptors,
					DisplayPercent,
					JudgeCounter,
					JudgmentText,
					PlayerInfo,
					BGBrightness,
					MeasureLines,
					ReceptorSize,
					ScreenFilter
				}
			),
			--todo:this
			--todo: pages
			section("Other minor elements", {})
		}
	)
	local graphics =
		tab(
		"Graphics",
		{
			section(
				"Essential options",
				{
					optionLines.Theme,
					optionLines.DisplayMode,
					optionLines.AspectRatio,
					optionLines.Resolution,
					optionLines.OverscanCorrection
				}
			),
			section(
				"Performance",
				{
					optionLines.DisplayColorDepth,
					optionLines.TextureColorDepth,
					optionLines.MovieColorDepth,
					optionLines.MaxTextureResolution,
					optionLines.HighResolutionTextures,
					optionLines.RefreshRate,
					optionLines.Vsync
				}
			)
		}
	)
	local controls =
		tab(
		"Controls",
		{
			--todo:this
			section("Keymappings", {keymappingOptionLineData("Menu left")}),
			section("Other binding settings", {})
		}
	)
	tabs = {
		gameplay,
		graphics,
		controls
	}
end
--config vars
local optionLineFont = {
	scale = 0.5,
	color = getMainColor("positive"),
	padding = {
		x = 0,
		y = 0
	}
}
local tabRightSides = {}
local lineHeight = 18
local sectionNameWidth = 180
local tabLabelScale = 0.9
local titleLabelScale = 1.3
local tabLabelHeight = 25

-- Mutable global state for this screen
local currentTab = nil

local t =
	Def.ActorFrame {
	LeftClickMessageCommand = function()
		MESSAGEMAN:Broadcast("MouseLeftClick")
	end,
	RightClickMessageCommand = function()
		MESSAGEMAN:Broadcast("MouseRightClick")
	end
}

-- background and separating line
t[#t + 1] = LoadActor("_menulines")

-- Added later to the left side
local welcomeText =
	Widg.Container {
	y = SCREEN_HEIGHT * 0.4
}
welcomeText:add(
	Widg.Label(
		setmetatable(
			{
				text = "Welcome to the options screen!",
				halign = 0
			},
			{__index = optionLineFont}
		)
	)
)
welcomeText:add(
	Widg.Label(
		setmetatable(
			{
				y = lineHeight,
				halign = 0,
				width = SCREEN_WIDTH * 0.6,
				text = "Use the arrow keys or the mouse to make selections, and use the [back] key to get out of the current set of options"
			},
			{__index = optionLineFont}
		)
	)
)

-- do the left side
local leftSide =
	Widg.Container {
	x = 115,
	y = SCREEN_CENTER_Y - 82
}

local tabsContainer =
	Widg.Container {
	y = tabLabelHeight * (titleLabelScale / tabLabelScale)
}
local highlightButton = function(button)
	button.label.actor:diffusealpha(1)
end
local unhighlightButton = function(button)
	button.label.actor:diffusealpha(button.selected and 1.0 or 0.7)
end
local function changeTab(idx)
	currentTab = idx
	for h, v in ipairs(tabRightSides) do
		local current = h == currentTab
		v.actor:visible(current):y(current and 0 or math.huge)
		tabsContainer[h].button.selected = current
		unhighlightButton(tabsContainer[h].button)
	end
	welcomeText.actor:visible(not currentTab)
end
do
	leftSide[#leftSide + 1] =
		Widg.Label {
		color = getMainColor("positive"),
		text = "Options",
		scale = titleLabelScale
	}

	do
		local i = 0
		local font = {
			scale = tabLabelScale,
			color = getMainColor("positive"),
			padding = {
				x = 0,
				y = 0
			}
		}
		for j, tabData in ipairs(tabs) do
			local name = tabData.name
			local tab = Widg.Container {}
			tab.button =
				Widg.Button {
				text = name,
				y = (j - 1) * tabLabelHeight,
				width = 130,
				halign = 1,
				font = font,
				alpha = 0,
				onInit = unhighlightButton,
				onUnhighlight = unhighlightButton,
				onHighlight = highlightButton,
				onClick = function(button)
					changeTab(j)
				end
			}
			tab.button.selected = false
			tab[#tab + 1] = tab.button
			tabsContainer:add(tab)
			i = i + 1
		end
		leftSide[#leftSide + 1] = tabsContainer
	end
end
t[#t + 1] = leftSide

local optionLineBuilders = defaultOptionLineBuilders(optionLineFont, lineHeight, sectionNameWidth)
-- right side
local rightSide =
	Widg.Container {
	x = SCREEN_WIDTH * 0.33,
	y = 0
}
do
	rightSide[#rightSide + 1] = welcomeText
	local optionSections = Widg.Container {}
	local counter = 0
	for i, tab in ipairs(tabs) do
		local tabDef =
			Widg.Container {
			visible = false
		}
		local lineCounter = 0
		for j, section in ipairs(tab.sections) do
			local sectionDef =
				Widg.Container {
				y = lineHeight * lineCounter
			}
			sectionDef:add(
				Widg.Label(
					setmetatable(
						{
							--skip a line for section spacing
							y = lineHeight * 3 / 2,
							text = section.name,
							halign = 0,
							width = sectionNameWidth
						},
						{__index = optionLineFont}
					)
				)
			)
			--skip a line for section spacing
			local sectionLineCounter = 2
			for h, line in ipairs(section) do
				local lineDef =
					Widg.Container {
					y = lineHeight * sectionLineCounter
				}
				lineDef:add(makeOptionLine(line, optionLineBuilders))
				sectionDef[#sectionDef + 1] = lineDef
				sectionLineCounter = sectionLineCounter + 1
			end
			tabDef[#tabDef + 1] = sectionDef
			lineCounter = sectionLineCounter + lineCounter
		end
		optionSections[#optionSections + 1] = tabDef
		tabRightSides[i] = tabDef
	end
	rightSide[#rightSide + 1] = optionSections
end
t[#t + 1] = rightSide
-- Add cursor at the end so it's over the rest
t[#t + 1] = LoadActor("_cursor")

return Def.ActorFrame {t}
