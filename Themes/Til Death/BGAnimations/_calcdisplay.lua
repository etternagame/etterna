local oldWidth = capWideScale(280, 300)
local plotWidth, plotHeight = capWideScale(300,450), 160
local plotX, plotY = oldWidth+3 + plotWidth/2, -20 + plotHeight/2
local highest = 0
local lowest = 0
local lowerGraphMax = 0
local upperGraphMax = 0
local lowerGraphMin = 0
local upperGraphMin = 0
local bgalpha = 0.9
local enabled = false
local ssrGraphActive = false
local song
local steps
local finalSecond = 0 -- only used if its references below are uncommented
local graphVecs = {}
local ssrs = {}
local activeModGroup = 1
local debugstrings

-- bg actors for mouse hover stuff
local topgraph = nil
local bottomgraph = nil

local function fitX(x, lastX) -- Scale time values to fit within plot width.
	if lastX == 0 then
		return 0
	end
	return x / lastX * plotWidth - plotWidth / 2
end

-- a scaling function which outputs a percentage based on a given scale
local function scale(x, lower, upper, scaledMin, scaledMax)
    local perc = (x - lower) / (upper - lower)
    return perc * (scaledMax - scaledMin) + scaledMin
end

-- scale values to vertical positions within the top graph
local function fitY1(y) -- scale for upper graph
    if y < upperGraphMin then y = upperGraphMin + 0.005 end
    local num = scale(y, upperGraphMin, upperGraphMax, 0, 1)
    local out = -1 * num * plotHeight
    return out
end

-- scale values to vertical positions within the lower graph
local function fitY2(y, lb, ub)
    if lb == nil then lb = lowest end
    if ub == nil then ub = highest end
    local num = scale(y, lb, ub, 0, 1)
    local out = -1 * num * plotHeight + plotHeight/2
    return out
end

-- convert a percentage (distance horizontally across the graph) to an index 
local function _internalConvPercToInd(x, vec)
    local output = x
    if output < 0 then output = 0 end
    if output > 1 then output = 1 end

    local ind = notShit.round(output * #vec[1])
    if ind < 1 then ind = 1 end
    return ind
end

-- upper graph percentage to index
local function convertPercentToIndex(x)
    return _internalConvPercToInd(x, ssrs)
end

-- lower graph percentage to index
local function convertPercentToIndexForMods(x)
    return _internalConvPercToInd(x, graphVecs["JS"])
end

-- transforms the position of the mouse from the cd graph to the calc info graph
local function transformPosition(pos, w, px)
    distanceAcrossOriginal = (pos - px) / w
    out = distanceAcrossOriginal * plotWidth - plotWidth/2
    return out
end

-- for SSR graph generator, modify these constants
local ssrLowerBoundWife = 0.90 -- left end of the graph
local ssrUpperBoundWife = 0.97 -- right end of the graph
local ssrResolution = 200 -- higher number = higher resolution graph (and lag)

local function produceThisManySSRs(steps, rate)
    local count = ssrResolution
    if count < 10 then count = 10 end
    local output = {}
    for j = 1,8 do output[j] = {0,0,0,0,0,0,0,0} end

    for i = 1, count do
        local values = steps:GetSSRs(rate, ssrLowerBoundWife + ((ssrUpperBoundWife - ssrLowerBoundWife) / count) * i)
        for j = 1,8 do
            output[j][i] = values[j]
        end
    end

    return output
end

local function getGraphForSteps(steps)
    local output = produceThisManySSRs(steps, getCurRateValue())

    highest = output[1][1]
    lowest = output[1][1]
    for ss,vals in ipairs(output) do
        for ind,val in ipairs(vals) do
            if val > highest then highest = val end
            if val < lowest then lowest = val end
        end
    end
    lowest = lowest - 1
    highest = highest + 1
    return output
end

--[[
    Calc Debug Enums have really long names and saying things like CalcPatternMod_.... over and over is boring
    So instead what we can do is confuse the reader and programmer by messing with the enum string representation
    Turning "CalcPatternMod_JS" into just "JS" is both very clean and very confusing
    But it's for the better, I promise
]]
local function shortenEnum(prefix, e)
    return e:gsub(prefix.."_", "")
end

-- this is a huge nested table of all the calc debug enums
local CalcDebugTypes = {
    CalcPatternMod = CalcPatternMod,
    CalcDiffValue = CalcDiffValue,
    CalcDebugMisc = CalcDebugMisc,
}

-- list of all additional enums to include in the upper graph
-- it is assumed these are members of CalcDebugMisc
local miscToUpperMods = {
    StamMod = true,
    JackStamMod = true,
}

-- list of all additional enums to include in the lower graph
-- it is assumed these are members of CalcDebugMisc
local miscToLowerMods = {
    Pts = true,
    PtLoss = true,
    JackPtLoss = true,
}

-- this list is used for functional purposes to keep the order of the lists generated in a certain order
-- particularly, it's the order determined by the enums on the c++ side
-- you have to see it to believe it, but it really does work
local orderedExtraUpperMods = {}
for i, mod in pairs(CalcDebugMisc) do
    local mod = shortenEnum("CalcDebugMisc", mod)
    if miscToUpperMods[mod] then
        orderedExtraUpperMods[#orderedExtraUpperMods+1] = mod
    end
end

-- same as immediately above
local orderedExtraLowerMods = {}
for i, mod in pairs(CalcDebugMisc) do
    local mod = shortenEnum("CalcDebugMisc", mod)
    if miscToLowerMods[mod] then
        orderedExtraLowerMods[#orderedExtraLowerMods+1] = mod
    end
end

-- specify enum names as tables here
-- any number allowed
-- there is no order to anything in the groups, only the groups themselves
local debugGroups = {
    {   -- Group 1
        Stream = true,
        OHTrill = true,
        VOHTrill = true,
		OHJumpMod = true,
		Roll = true,
        StamMod = true,
    },
    {   -- Group 2
        JS = true,
        StamMod = true,
        OHJumpMod = true,
    },
	{   -- Group 3
        HS = true,
        StamMod = true,
        OHJumpMod = true,
	},
    {   -- Group 4
        CJ = true,
        CJDensity = true,
        CJOHJump = true,
		StamMod = true,
    },
    {   -- Group 5
		Roll = true,
		WideRangeRoll = true,
		WideRangeJumptrill = true,
	},
    {   -- Group 6
        Chaos = true,
        FlamJam = true,
        TheThing = true,
        Balance = true,
        WideRangeBalance = true,
        WideRangeAnchor = true,
    },
	{   -- Group 7
        RanMan = true,
    },
	{   -- Group 8
        OHJumpMod = true,
	},
    {   -- Group 9
        TotalPatternMod = true,
    },
    {   -- Group 10

    },
    {   -- Group 11
        Chaos = true,
        Roll = true,
    },
    [12] = { -- Group 12
        TheThing = true,
        TheThing2 = true,
    },
}

-- get a list of the mods that are active
-- indexes pointing to enum strings
local function getActiveDebugMods()
    local output = {}

    if activeModGroup > #debugGroups or activeModGroup < 1 then return output end

    -- once for each hand, add it to the list
    for mod,_ in pairs(debugGroups[activeModGroup]) do
        output[#output+1] = mod
        output[#output+1] = mod
    end

    return output
end

-- responsible for updating all relevant values and then triggering the display message(s)
local function updateCoolStuff()
    song = GAMESTATE:GetCurrentSong()
    steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
    if song then
        finalSecond = GAMESTATE:GetCurrentSong():GetLastSecond() * 2
    end
    if steps then
        -- Only load SSRs if currently displaying them; this is a major slowdown
        if ssrGraphActive then
            ssrs = getGraphForSteps(steps)
        else
            ssrs = {}
        end
        lowerGraphMax = 0
        local bap = steps:GetCalcDebugOutput()
        debugstrings = steps:GetDebugStrings()

        -- for each debug output type and its corresponding list of values
        for debugtype, sublist in pairs(CalcDebugTypes) do
            -- for each value in that list
            for i = 1, #sublist do

                -- translate the output list to our "cleaner" format
                local modname = shortenEnum(debugtype, sublist[i])
                graphVecs[modname] = {}

                -- for each hand
                for h = 1, 2 do
                    graphVecs[modname][h] = bap[debugtype][modname][h]

                    -- we set the bound of the lower graph to the max value of all debug output for it
                    if debugtype == "CalcDiffValue" then
                        for j = 1, #graphVecs[modname][h] do
                            local val = graphVecs[modname][h][j]
                            if val > lowerGraphMax then lowerGraphMax = val end
                        end
                    end
                end
            end
        end

        upperGraphMin = 0.3
        upperGraphMax = 1.25
    else
        graphVecs = {}
    end

    local mods = getActiveDebugMods()
    MESSAGEMAN:Broadcast("UpdateAverages", {mods = mods})
end

--[[
    Active modgroups are groups of Calc Debug Mods that selectively show
    We shouldn't ever show all of them unless otherwise noted because that's a lot of info and it's laggy
    (in any event, though, an invalid activeModGroup value will result in all modgroups appearing)
    ((-1 is meant to be the all group value but isn't really designed to work))
]]
-- switch the active mod group directly to a number (no cap)
local function switchToGroup(num)
    if num == activeModGroup then
        -- activeModGroup = -1
    else
        activeModGroup = num
    end
    local mods = getActiveDebugMods()
    MESSAGEMAN:Broadcast("UpdateActiveMods", {mods = mods})
end

-- move the active mod group value in a direction (looping)
local function addToModGroup(direction)
    if activeModGroup == -1 then
        if direction < 0 then
            switchToGroup(#debugGroups)
        elseif direction > 0 then
            switchToGroup(1)
        end
    else
        local newg = (((activeModGroup) + direction) % (#debugGroups + 1))
        if newg == 0 then
            newg = direction > 0 and 1 or #debugGroups
        end
        switchToGroup(newg)
    end
end

-- toggle between SSR graph and CalcDiffValue graph
-- should only update the SSRs once unless changing songs (it resets in that case)
local function switchSSRGraph()
    ssrGraphActive = not ssrGraphActive
    if ssrGraphActive and #ssrs == 0 then
        ssrs = getGraphForSteps(steps)
        MESSAGEMAN:Broadcast("UpdateSSRLines")
    end
    MESSAGEMAN:Broadcast("UpdateActiveLowerGraph")
end

-- this will gather all the mod names and values for a specific given index
-- it produces a single string for the purpose of hover information
local function getDebugModsForIndex(modgroup, modgroupname, extramodgroup, index, isUpper)
    local modsToValues = {}
    local modText = ""
    local modNames = {}

    -- gather all mods and names and values and stuff to put them in the hover text
    for i, mod in pairs(modgroup) do
        local mod = shortenEnum(modgroupname, mod)
        for h = 1, 2 do
            local hand = h == 2 and "R" or "L"
            modsToValues[#modsToValues + 1] = graphVecs[mod][h]
            modNames[#modNames + 1] = mod..hand
        end
    end

    -- for each hand add the enum-less mods (handle them last)
    for mod, _ in pairs(extramodgroup) do
        local mod = shortenEnum("CalcDebugMisc", mod)
        for h = 1,2 do
            local hand = h == 2 and "R" or "L"
            modsToValues[#modsToValues + 1] = graphVecs[mod][h]
            modNames[#modNames + 1] = mod..hand
        end
    end
    
    -- carry out the final string production
    for k, v in pairs(modsToValues) do
        local namenoHand = modNames[k]:sub(1, #modNames[k]-1)
        if (isUpper and activeModGroup == -1 or debugGroups[activeModGroup][namenoHand]) or not isUpper then
            local name = modNames[k] and modNames[k] or ""
            local value = v[index] and v[index] or 0
            local txt = string.format(name..": %5.4f\n", value)
            modText = modText .. txt
        end
    end

    modText = modText:sub(1, #modText-1) -- remove the end whitespace
    modText = modText .. "\n" .. index
    return modText
end

-- input handler
-- note: returning true in a handler stops input from being sent anywhere else
local function yetAnotherInputCallback(event)
    if event.type == "InputEventType_FirstPress" then
        if event.DeviceInput.button == "DeviceButton_mousewheel up" then
            if isOver(topgraph) then
                addToModGroup(1)
                return true
            elseif isOver(bottomgraph) then
                switchSSRGraph()
                return true
            end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
            if isOver(topgraph) then
                addToModGroup(-1)
                return true
            elseif isOver(bottomgraph) then
                switchSSRGraph()
                return true
            end
        end

        local CtrlPressed = INPUTFILTER:IsControlPressed()
        if tonumber(event.char) and CtrlPressed and enabled then
            local num = tonumber(event.char)
            if num == 0 then
                switchSSRGraph()
            end
        end
	end
	return false
end

local o =
	Def.ActorFrame {
    Name = "notChordDensityGraph", -- it's not the chord density graph
	OnCommand = function(self)
        self:xy(plotX, plotY)
    end,
    OffCommand = function(self)
        self:playcommand("CalcInfoOff")
    end,
    CalcInfoOnMessageCommand = function(self)
        updateCoolStuff()
        self:visible(true)
        enabled = true
        SCREENMAN:GetTopScreen():GetMusicWheel():visible(false)
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end,
    CalcInfoOffMessageCommand = function(self)
        self:visible(false)
        enabled = false
        SCREENMAN:GetTopScreen():GetMusicWheel():visible(true)
    end,
    CurrentStepsP1ChangedMessageCommand = function(self)
        if not enabled then return end
        updateCoolStuff()
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("CurrentStepsP1Changed")
    end,
    Def.Quad {
        Name = "GraphPos",
        InitCommand = function(self)
            self:xy(-plotWidth/2, -20)
            self:zoomto(0, 0):diffuse(color("1,1,1,1")):halign(0):draworder(1100):halign(0):diffusealpha(0.1)
        end,
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(yetAnotherInputCallback)
        end
    }
}

o[#o + 1] = Def.Quad {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
    end,
    DoTheThingCommand = function(self)
        self:visible(song ~= nil)
    end,
    HighlightCommand = function(self)
        local txt = self:GetParent():GetChild("DebugStringText")
        if isOver(self) then
            local mx = INPUTFILTER:GetMouseX()
            local ypos = INPUTFILTER:GetMouseY() - self:GetParent():GetY()
            
            local w = self:GetZoomedWidth() * self:GetParent():GetTrueZoom()
            local leftEnd = self:GetTrueX() - (self:GetHAlign() * w)
            local rightEnd = self:GetTrueX() + w - (self:GetHAlign() * w)
            local perc = (mx - leftEnd) / (rightEnd - leftEnd)
            local goodXPos = -plotWidth/2 + perc * plotWidth
			
            txt:visible(true)
			txt:x(goodXPos + 36)
            txt:y(ypos - 40)

            local index = convertPercentToIndexForMods(perc)
            txt:settext(debugstrings[index])
		else
            txt:visible(false)
		end
	end
}

-- graph bg
o[#o + 1] = Def.Quad {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
        topgraph = self
    end,
    DoTheThingCommand = function(self)
        self:visible(song ~= nil)
    end,
    HighlightCommand = function(self)
		local bar = self:GetParent():GetChild("GraphSeekBar")
        local txt = self:GetParent():GetChild("GraphText")
        local bg = self:GetParent():GetChild("GraphTextBG")
        if isOver(self) then
            local mx = INPUTFILTER:GetMouseX()
            local ypos = INPUTFILTER:GetMouseY() - self:GetParent():GetY()
            
            local w = self:GetZoomedWidth() * self:GetParent():GetTrueZoom()
            local leftEnd = self:GetTrueX() - (self:GetHAlign() * w)
            local rightEnd = self:GetTrueX() + w - (self:GetHAlign() * w)
            local perc = (mx - leftEnd) / (rightEnd - leftEnd)
            local goodXPos = -plotWidth/2 + perc * plotWidth

			bar:visible(true)
            txt:visible(true)
            bg:visible(true)
			bar:x(goodXPos)
			txt:x(goodXPos - 4)
            txt:y(ypos)
            bg:zoomto(txt:GetZoomedWidth() + 6, txt:GetZoomedHeight() + 6)
            bg:x(goodXPos)
            bg:y(ypos)

            local index = convertPercentToIndexForMods(perc)
            local modText = getDebugModsForIndex(CalcPatternMod, "CalcPatternMod", miscToUpperMods, index, true)
            txt:settext(modText)
		else
			bar:visible(false)
            txt:visible(false)
            bg:visible(false)
		end
	end
}

-- mod group indicators appears top left of top graph bg
o[#o+1] = LoadFont("Common Normal") .. {
    Name = "G1Group",
    InitCommand = function(self)
        self:xy(-plotWidth/2,-plotHeight/2)
        self:halign(0):valign(0)
        self:zoom(0.25)
        self:settextf("Group %d", activeModGroup)
    end,
    UpdateActiveModsMessageCommand = function(self)
        self:settextf("Group %d", activeModGroup)
    end
}

-- second bg
o[#o + 1] = Def.Quad {
    Name = "G2BG",
    InitCommand = function(self)
        self:y(plotHeight + 5)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
        bottomgraph = self
    end,
    DoTheThingCommand = function(self)
        self:visible(song ~= nil)
    end,
    HighlightCommand = function(self)
		local bar = self:GetParent():GetChild("Seek2")
        local txt = self:GetParent():GetChild("Seektext2")
        local bg = self:GetParent():GetChild("Seektext2BG")
        if isOver(self) then
            local mx = INPUTFILTER:GetMouseX()
            local ypos = INPUTFILTER:GetMouseY() - self:GetParent():GetY()
            
            local w = self:GetZoomedWidth() * self:GetParent():GetTrueZoom()
            local leftEnd = self:GetTrueX() - (self:GetHAlign() * w)
            local rightEnd = self:GetTrueX() + w - (self:GetHAlign() * w)
            local perc = (mx - leftEnd) / (rightEnd - leftEnd)
            local goodXPos = -plotWidth/2 + perc * plotWidth

			bar:visible(true)
            txt:visible(true)
            bg:visible(true)
			bar:x(goodXPos)
			txt:x(goodXPos - 4)
            txt:y(ypos)
            bg:zoomto(txt:GetZoomedWidth() + 6, txt:GetZoomedHeight() + 6)
            bg:x(goodXPos)
            bg:y(ypos + 3)
            
            if not ssrGraphActive then
                local index = convertPercentToIndexForMods(perc)
                local modText = getDebugModsForIndex(CalcDiffValue, "CalcDiffValue", miscToLowerMods, index, false)
                txt:settext(modText)
            else
                local ssrindex = convertPercentToIndex(perc)
                -- The names here are made under the assumption the skillsets and their positions never change
                local ssrAtIndex = {
                    ssrs[1][ssrindex], -- overall
                    ssrs[2][ssrindex], -- stream
                    ssrs[3][ssrindex], -- jumpstream
                    ssrs[4][ssrindex], -- handstream
                    ssrs[5][ssrindex], -- stamina
                    ssrs[6][ssrindex], -- jackspeed
                    ssrs[7][ssrindex], -- chordjack
                    ssrs[8][ssrindex], -- technical
                }
                local ssrtext = string.format("Percent: %5.4f\n", (ssrLowerBoundWife + (ssrUpperBoundWife-ssrLowerBoundWife)*perc)*100)
                for i, ss in ipairs(ms.SkillSets) do
                    ssrtext = ssrtext .. string.format("%s: %.2f\n", ss, ssrAtIndex[i])
                end
                ssrtext = ssrtext:sub(1, #ssrtext-1) -- remove the end whitespace
                txt:settext(ssrtext)
            end
            
		else
			bar:visible(false)
            txt:visible(false)
            bg:visible(false)
		end
	end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:xy(-plotWidth/4, plotHeight + 5 + plotHeight/2 + 35)
        self:zoom(0.55)
        self:settext("")
        self:maxwidth(plotWidth * 3/4 / 0.55)
        self:halign(0)
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            title = song:GetDisplayFullTitle()
            artist = song:GetDisplayArtist()
            self:settext(title .. "\n  ~" .. artist)
        end
    end
}

--[[ enum mapping for downscaler things:
    this list has order and should match the enums used
]]
local modnames = {
    -- CalcPatternMod shortnames
    "stl",
    "str",
    "jsl",
    "jsr",
    --"jssl",
    --"jssr",
    --"jsjl",
    --"jsjr",
    "hsl",
    "hsr",
    --"hssl",
    --"hssr",
    --"hsjl",
    --"hsjr",
    "cjl",
    "cjr",
    --"cjsl",
    --"cjsr",
    --"cjjl",
    --"cjjr",
    "cjdl",
    "cjdr",
    "ohjl",
    "ohjr",
    --"ohjbpr",
    --"ohjbpl",
    --"ohjpcl",
    --"ohjpcr",
    --"ohjscl",
    --"ohjscr",
    --"ohjmsl",
    --"ohjmsr",
    --"ohjcctl",
    --"ohjcctr",
    --"ohjhtl",
    --"ohjhtr",
    --"cjohjl",
    --"cjohjr",
    --"cjohjpcl",
    --"cjohjpcr",
    --"cjohjscl",
    --"cjohjscr",
    "blncl",
    "blncr",
    "rolll",
    "rollr",    
    "ohtl",
    "ohtr",
    "vohtl",
    "vohtr",
    "cl",
    "cr",
    "fcl",
    "fcr",
    "wrrl",
    "wrrr",
    "wrjtl",
    "wrjtr",
    "wrbl",
    "wrbr",
    "wral",
    "wrar",
    "ttl",
    "ttr",
    "tt2l",
    "tt2r",
    "rml",
    "rmr",
    --"rll",
    --"rlr",
    --"rall",
    --"ralr",
    --"ralml",
    --"ralmr",
    --"rjl",
    --"rjr",
    --"rohtl",
    --"rohtr",
    --"rosl",
    --"rosr",
    --"rpal",
    --"rpar",
    --"rpol",
    --"rpor",
    --"rpohtl",
    --"rpohtr",
    --"rposl",
    --"rposr",
    --"rpjl",
    --"rpjr",
    --"totpml",
    --"totpmr",


    -- CalcPatternMods above this line
    -- CalcDebugMisc mods meant for only the top graph:
    -- (this list should match the miscToUpperMods list)
    "sl",
    "sr",
    "jksl",
    "jksr",
}

-- this list has order
-- try to keep it exactly in the order of the enums used :)
local modColors = {
    -- CalcDebugPattern Colors
    color(".3,1.3,1"),      -- cyan			= stream left
    color(".3,1.3,0.9"),	-- cyan				 (right)
	color("1,0,1"),     	-- purple       = jumpstream left
    color("1,0.3,1"),   	-- light purple      (right)
	--color("0,1,1"),			-- cyan			= jumpstream stream
	--color("0,0.8,1"),		-- light blue		 (right)
	--color("1,0,0"),			-- red			= jumpstream jack
	--color("1,0.2,0"),		-- orange1			 (right)
    color("0.6,0.6,0"),     -- dark yellow  = handstream left
    color("0.6,0.6,0"),     -- dark yellow       (right)
	--color("0,1,1"),			-- cyan			= handstream stream
	--color("0,0.8,1"),		-- light blue		 (right)
	--color("1,0,0"),			-- red			= handstream jack
	--color("1,0.2,0"),		-- orange1			 (right)
    color("1.4,1.3,1"),     -- white 		= chordjack left
    color("1.4,1.3,0.9"),   -- white			 (right)
	--color("0,1,1"),			-- cyan			= chordjack stream
	--color("0,0.8,1"),		-- light blue		 (right)
	--color("1,0,0"),			-- red			= chordjack jack
	--color("1,0.2,0"),		-- orange1			 (right)
	color("1,1,0"),			-- yellow		= cjdensity left
    color("1,1,0"),			-- yellow			 (right)
    color("1,0.4,0"),       -- orange2		= ohjump left
    color("1,0.4,0"), 		-- orange2        	 (right)
	--color("1,1,1"),			-- ohjbp
	--color("1,1,1"),
	--color("1,1,1"),			-- ohjpc
	--color("1,1,1"),
	--color("1,1,1"),			-- ohjsc
	--color("1,1,1"),
	--color("1,1,1"),			-- ohjms
	--color("1,1,1"),
	--color("1,1,1"),			-- ohjcct
	--color("1,1,1"),
	--color("1,1,1"),			-- ohjht
	--color("1,1,1"),
    --color("1,0.4,0"),		-- orange2		= cjohj left
    --color("1,0.4,0"),		-- orange2			 (right)
	--color("1,1,1"),			-- cjohjpc
	--color("1,1,1"),
	--color("1,1,1"),			-- cjohjsc
	--color("1,1,1"),
    color("0.2,0.2,1"),     -- blue         = balance left
    color("0.3,0.3,0.9"),   -- light blue        (right)
    color("0,1,0"),         -- green        = roll left
    color("0.3,0.9,0.3"),   -- light green       (right)
    color(".8,1.3,1"),      -- whiteblue	= oht left
    color(".8,1.3,0.9"),	-- whiteblue		 (right)
    color("1,0,1"),         -- purple       = voht left
    color("1,0,1"),         -- purple            (right)
    color(".4,0.9,0.3"),    -- green		= chaos left
    color(".4,0.9,0.3"),	-- green			 (right)
    color(".4,0.5,0.59"),   -- teal			= flamjam left
    color(".4,0.3,0.49"),   -- dark purple		 (right)
    color("1,0.2,0"),		-- red			= wrr left
    color("1,0.2,0"),		-- red				 (right)
    color("1,0.5,0"),		-- orange		= wrjt left
    color("1,0.5,0"),		-- orange			 (right)
    color("0.7,1,0.2"),		-- leme			= wrb left
    color("0.7,1,0.2"),		-- leme				 (right)
    color("0.7,1,0.1"),		-- leme			= wra left
    color("0.7,1,0.1"),		-- leme				 (right)
    color("0,0.8,1"),		-- light blue	= thething left
    color("0,0.8,1"),		-- light blue		 (right)
    color("0,0.6,1"),       -- darkish blue = thething2 left
    color("0,0.6,1"),       --                  (right)
	color("0.2,1,1"),		-- light blue	= ranman left
	color("0.2,1,1"),		-- light blue		 (right)
	--color("1,1,1"),			-- rl
	--color("1,1,1"),
	--color("1,1,1"),			-- ral
	--color("1,1,1"),
	--color("1,1,1"),			-- ralm
	--color("1,1,1"),
	--color("1,1,1"),			-- rj
	--color("1,1,1"),
	--color("1,1,1"),			-- roht
	--color("1,1,1"),
	--color("1,1,1"),			-- ros
	--color("1,1,1"),
	--color("1,1,1"),			-- rpa
	--color("1,1,1"),
	--color("1,1,1"),			-- rpo
	--color("1,1,1"),
	--color("1,1,1"),			-- rpoht
	--color("1,1,1"),
	--color("1,1,1"),			-- rpos
	--color("1,1,1"),
	--color("1,1,1"),			-- rpj
    --color("1,1,1"),
    --color("0.7,1,0"),		-- lime			= totalpatternmod left
    --color("0.7,1,0"),		-- lime				 (right)


    -- place CalcPatternMod Colors above this line
    -- MISC MODS START HERE (same order as miscToUpperMods)
    color("0.7,1,0"),		-- lime			= stam left
    color("0.7,1,0"),		-- lime				 (right)
    color("0.7,1,0"),		-- lime			= jackstam left
    color("0.7,1,0"),		-- lime				 (right)
}

-- a remapping of modnames to colors
local modToColor = {}
-- a remapping of modnames to shortnames
local modToShortname = {}
for i, mod in pairs(CalcPatternMod) do
    local mod = shortenEnum("CalcPatternMod", mod)
    modToColor[mod.."L"] = modColors[i*2 - 1]
    modToColor[mod.."R"] = modColors[i*2]
    modToShortname[mod.."L"] = modnames[i*2 - 1]
    modToShortname[mod.."R"] = modnames[i*2]
end
do -- scope hahaha
    local i = 1
    for _, mod in pairs(orderedExtraUpperMods) do
        modToColor[mod.."L"] = modColors[#CalcPatternMod*2 + i*2 - 1]
        modToColor[mod.."R"] = modColors[#CalcPatternMod*2 + i*2]
        modToShortname[mod.."L"] = modnames[#CalcPatternMod*2 + i*2 - 1]
        modToShortname[mod.."R"] = modnames[#CalcPatternMod*2 + i*2]
        i = i + 1
    end
end

-- top graph average text
local function makeskillsetlabeltext(i)
    return LoadFont("Common Normal") .. {
        Name = "SSLabel"..i,
        InitCommand = function(self)
            local xspace = 42   -- this is gonna look like shit on 4:3 no matter what so w.e
            self:xy(-plotWidth/2 + 5 + math.floor((i-1)/4) * xspace, plotHeight/3.3 + ((i-1)%4)*8.5):halign(0)
            self:zoom(0.3)
            self:settext("")
            self:maxwidth(120)
        end,
        UpdateAveragesMessageCommand = function(self, params)
            if song then
                local mod = nil
                local hand = (i+1) % 2 + 1

                -- update and show only if needed
                if params.mods[i] then
                    mod = params.mods[i]
                    self:diffusealpha(1)
                else
                    self:diffusealpha(0)
                    return
                end

                local modhand = mod .. (hand == 1 and "L" or "R")
                local shortname = modToShortname[modhand]
                local modcolor = modToColor[modhand]

                local ave
                local values = graphVecs[mod][hand]
                if not values or not values[1] then 
                    self:settext("")
                    return
                end
                if values[i] and #values > 0 then
                    ave = table.average(values)
                end
                if activeModGroup == -1 or (debugGroups[activeModGroup] and debugGroups[activeModGroup][mod]) then
                    self:diffuse(modcolor)
                else
                    self:diffuse(color(".3,.3,.3"))
                end
                if ave then
                    self:settextf("%s: %.3f", shortname, ave)
                else
                    self:settextf("%s: err", shortname)
                end
            end
        end,
        UpdateActiveModsMessageCommand = function(self, params)
            local mod = params.mods[i]
            -- if this group is selected and we want to show it off
            if activeModGroup == -1 or (debugGroups[activeModGroup] and debugGroups[activeModGroup][mod]) then
                self:playcommand("UpdateAverages", {mods = params.mods})
            else
                -- grey out unselected groups
                self:diffusealpha(0)
                --self:diffuse(color(".3,.3,.3"))
            end
        end
    }
end

-- lower graph average text
o[#o + 1] = LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:xy(-plotWidth/2 + 5, plotHeight/2 + 10):halign(0):valign(0)
        self:zoom(0.4)
        self:settext("")
    end,
    DoTheThingCommand = function(self)
        if song and enabled then
            self:settextf("Upper Bound: %.4f", lowerGraphMax)
        end
    end
}

local dotWidth = 0
local function setOffsetVerts(vt, x, y, c)
	vt[#vt + 1] = {{x - dotWidth, y + dotWidth, 0}, c}
	vt[#vt + 1] = {{x + dotWidth, y + dotWidth, 0}, c}
	vt[#vt + 1] = {{x + dotWidth, y - dotWidth, 0}, c}
	vt[#vt + 1] = {{x - dotWidth, y - dotWidth, 0}, c}
end

local function topGraphLine(mod, colorToUse, hand)
    return Def.ActorMultiVertex {
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                self:visible(true)
                local verts = {}
                local highest = 0

                -- hack to draw a line at 1.0
                if mod == "base_line" then
                    for i = 1, #graphVecs["JS"][1] do
                        local x = fitX(i, #graphVecs["JS"][1])
                        local y = fitY1(1)
                        y = y + plotHeight / 2
                        setOffsetVerts(verts, x, y, color("1,1,1"))
                    end
                    self:SetVertices(verts)
                    self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
                    return
                end

                local values = graphVecs[mod][hand]
                if not values or not values[1] then return end
                for i = 1, #values do
                    local x = fitX(i, #values) -- vector length based positioning
                    --local x = fitX(i, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY1(values[i])
                    y = y + plotHeight / 2
                    setOffsetVerts(verts, x, y, colorToUse) 
                end

                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}

                if activeModGroup == -1 or (debugGroups[activeModGroup] and debugGroups[activeModGroup][mod]) then
                    self:diffusealpha(1)
                else
                    -- hide unselected groups
                    if mod ~= "base_line" then
                        self:diffusealpha(0)
                    end
                end
            else
                self:visible(false)
            end
        end,
        UpdateActiveModsMessageCommand = function(self)
            -- if this group is selected and we want to show it off
            if activeModGroup == -1 or (debugGroups[activeModGroup] and debugGroups[activeModGroup][mod]) then
                self:diffusealpha(1)
            else
                -- hide unselected groups
                if mod ~= "base_line" then
                    self:diffusealpha(0)
                end
            end
        end
    }
end

local function bottomGraphLineMSD(mod, colorToUse, hand)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                self:visible(not ssrGraphActive)

                local verts = {}
                local values = graphVecs[mod][hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    local x = fitX(i, #values) -- vector length based positioning
                    --local x = fitX(i, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i], lowerGraphMin, lowerGraphMax)

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                self:visible(not ssrGraphActive)
            end
        end
    }
end

local function bottomGraphLineSSR(lineNum, colorToUse)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled and #ssrs > 0 then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

                self:visible(ssrGraphActive)
                local verts = {}

                for i = 1, #ssrs[lineNum] do
                    local x = fitX(i, #ssrs[lineNum]) -- vector length based positioning
                    --local x = fitX(i, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY2(ssrs[lineNum][i])

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                self:visible(ssrGraphActive)
            end
        end,
        UpdateSSRLinesMessageCommand = function(self)
            self:playcommand("DoTheThing")
        end
    }
end

local skillsetColors = {
    color("#7d6b91"),   -- stream
    color("#8481db"),   -- jumpstream
    color("#995fa3"),   -- handstream
    color("#f2b5fa"),   -- stamina
    color("#6c969d"),   -- jack
    color("#a5f8d3"),   -- chordjack
    color("#b0cec2"),    -- tech
    color("#b0cec2"),    -- tech 2 (used for something else dont worry)
}

local miscColors = {
    color("1,0,0"),     -- ptloss
    color("1,0,0"),     
    color("1,0.4,0"),   -- jackptloss
    color("1,0.4,0"),
}

-- upper mod lines and text
-- we do the hand loop inner so the text lines up with hands next to each other
do -- scoping
    local i
    for i, mod in pairs(CalcPatternMod) do
        for h = 1,2 do
            local modname = shortenEnum("CalcPatternMod", mod)
            o[#o+1] = topGraphLine(modname, modColors[(i * 2) - (h % 2)], h)
            --o[#o+1] = makeskillsetlabeltext((i * 2) - (h % 2), modname, h)
        end
    end
    i = 1
    for mod, _ in pairs(miscToUpperMods) do
        for h = 1,2 do
            -- dont have to shorten enum here because i did something dumb
            o[#o+1] = topGraphLine(mod, modColors[(#CalcPatternMod * 2) + i], h)
            --o[#o+1] = makeskillsetlabeltext((#CalcPatternMod * 2) + i, mod, h)
        end
        i = i + 1
    end
end

-- backing to the text
o[#o+1] = Def.Quad {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight/4)
        self:y(plotHeight/2 - plotHeight/8)
        self:diffuse(color(".1,.1,.1,.8"))
    end
}

-- create 40 slots for text on top
-- there is room for about 44 at the time of writing
for i = 1,40 do
    o[#o+1] = makeskillsetlabeltext(i)
end

-- upper graph 1.0 baseline
o[#o+1] = topGraphLine("base_line", color("1,1,1,1"))

-- lower mod lines and stuff
do -- scoping
    local i
    for i, mod in pairs(CalcDiffValue) do
        local modname = shortenEnum("CalcDiffValue", mod)
        for h = 1,2 do
                o[#o+1] = bottomGraphLineMSD(modname, skillsetColors[(i * 2) - (h % 2)], h)
        end
    end
    i = 1
    for mod, _ in pairs(miscToLowerMods) do
        for h = 1,2 do
            o[#o+1] = bottomGraphLineMSD(mod, miscColors[(i-1)*2 + h], h)
        end
        i = i + 1
    end
end

-- SSR skillset lines
for i = 1,8 do
    o[#o+1] = bottomGraphLineSSR(i, skillsetColors[i])
end

-- a bunch of things for stuff and things
o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "Seektext1",
    InitCommand = function(self)
        self:y(8):valign(1):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
    end,
    UpdatePositionCommand = function(self, params)
        self:x(transformPosition(params.pos, params.w, params.px) - 5)
    end
}

o[#o + 1] = Def.Quad {
    Name = "Seek1",
    InitCommand = function(self)
        self:zoomto(1, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end,
    UpdatePositionCommand = function(self, params)
        self:x(transformPosition(params.pos, params.w, params.px))
    end
}

o[#o + 1] = Def.Quad {
    Name = "Seektext2BG",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("0,0,0,.4")):zoomto(20,20)
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "Seektext2",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.4)
    end
}

o[#o + 1] = Def.Quad {
    Name = "Seek2",
    InitCommand = function(self)
        self:y(plotHeight+5)
        self:zoomto(1, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end
}

o[#o + 1] = Def.Quad {
    Name = "GraphTextBG",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):halign(1):draworder(1100):diffuse(color("0,0,0,.4")):zoomto(20,20)
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "GraphText",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.4)
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "DebugStringText",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.5)
    end
}

o[#o + 1] = Def.Quad {
    Name = "GraphSeekBar",
    InitCommand = function(self)
        self:zoomto(1, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end
}


return o