-- for cdgraph fitting
--[[
local plotWidth, plotHeight = capWideScale(280, 300), 40
local plotX, plotY = plotWidth/2, 0
]]
-- for standard fitting
local oldWidth = capWideScale(280, 300)
local plotWidth, plotHeight = capWideScale(300,450), 160
local plotX, plotY = oldWidth+3 + plotWidth/2, -20 + plotHeight/2
local dotDims, plotMargin = 2, 4
local highest = 0
local lowest = 0
local lowerGraphMax = 0
local upperGraphMax = 0
local lowerGraphMin = 0
local upperGraphMin = 0
local baralpha = 0.2
local bgalpha = 0.9
local textzoom = 0.35
local enabled = false
local modvaluescaler = 150
local song
local steps
local finalSecond
local graphVecs = {}
local ssrs = {}



local function fitX(x, lastX) -- Scale time values to fit within plot width.
	if lastX == 0 then
		return 0
	end
	return x / lastX * plotWidth - plotWidth / 2
end

local function fitY(y) -- Scale diff values to fit within plot height
	return -plotHeight * y + ((1 - y) * modvaluescaler) + 10
end

local function scale(x, lower, upper, scaledMin, scaledMax) -- uhh
    local perc = (x - lower) / (upper - lower)
    return perc * (scaledMax - scaledMin) + scaledMin
end

local function fitY1(y) -- scale for upper graph
    local num = scale(y, upperGraphMin, upperGraphMax, 0, 1)
    local out = -1 * num * plotHeight
    return out
end

local function fitY2(y, lb, ub) -- scale for lower graph
    if lb == nil then lb = lowest end
    if ub == nil then ub = highest end
    local num = scale(y, lb, ub, 0, 1)
    local out = -1 * num * plotHeight + plotHeight/2
    return out
end

local function convertPercentToIndex(x)
    local output = x
    if output < 0 then output = 0 end
    if output > 1 then output = 1 end

    local ind = notShit.round(output * #ssrs[1])
    if ind < 1 then ind = 1 end
    return ind
end

local function convertPercentToIndexForMods(x)
    local output = x
    if output < 0 then output = 0 end
    if output > 1 then output = 1 end

    local ind = notShit.round(output * #graphVecs["Jump"][1])
    if ind < 1 then ind = 1 end
    return ind
end

local function HighlightUpdaterThing(self)
    if not enabled then return end
    --self:GetChild("G2BG"):queuecommand("Highlight")
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

-- :thinking:
local CalcDebugTypes = {
    CalcPatternMod = CalcPatternMod,
    CalcDiffValue = CalcDiffValue,
    CalcDebugMisc = CalcDebugMisc,
}

-- active mods on top graph
local activeMods = {}
for k,v in pairs(CalcPatternMod) do
    v = v:gsub("CalcPatternMod_", "")
    activeMods[#activeMods+1] = v
end
activeMods[#activeMods+1] = "StamMod"

-- specify enum names as tables here
-- only allowed to have 9
-- the 0th one is reserved for ssr graph toggle
local debugGroups = {
    {
        OHJump = true,
        Anchor = true
    },
    {
        Roll = true,
        HS = true
    },
    {
        Jump = true,
        CJ = true
    },
    {
        StreamMod = true,
        OHTrill = true,
    },
    {
        Chaos = true,
        FlamJam = true,
    },
    {
        WideRangeRoll = true,
        StamMod = true
    },
    {},
    {},
    [9] = {},
}

local function updateCoolStuff()
    song = GAMESTATE:GetCurrentSong()
    steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
    if song then
        finalSecond = GAMESTATE:GetCurrentSong():GetLastSecond() * 2
    end
    if steps then
        ssrs = getGraphForSteps(steps) -- maybe add back the new wrapper
        lowerGraphMax = 0
        local bap = steps:GetCalcDebugOutput()

        -- loop through types of debug output
        for k, v in pairs(CalcDebugTypes) do
            for i = 1, #v do        -- loop through specific mods
                local modname = v[i]:gsub(k.."_", "")
                graphVecs[modname] = {}
                for h = 1, 2 do     -- left/right hand loop
                    graphVecs[modname][h] = bap[k][modname][h]
                    if k == "CalcDiffValue" then
                        for j = 1, #graphVecs[modname][h] do
                            local val = graphVecs[modname][h][j]
                            if val > lowerGraphMax then lowerGraphMax = val end
                        end
                    end
                end
            end
        end

        upperGraphMin = 0.3
        upperGraphMax = 1.2
    else
        graphVecs = {}
    end
    MESSAGEMAN:Broadcast("UpdateAverages")
end

local activeModGroup = -1
-- just switches the active group of lines
-- all others are invis/grey
local function switchToGroup(num)
    if num == activeModGroup then
        activeModGroup = -1
    else
        activeModGroup = num
    end
    MESSAGEMAN:Broadcast("UpdateActiveMods")
end

local ssrGraphActive = false
local function switchSSRGraph()
    ssrGraphActive = not ssrGraphActive
    MESSAGEMAN:Broadcast("UpdateActiveLowerGraph")
end

local function yetAnotherInputCallback(event)
	if event.type == "InputEventType_FirstPress" then
        local CtrlPressed = INPUTFILTER:IsControlPressed()
        if tonumber(event.char) and CtrlPressed then
            local num = tonumber(event.char)
            if num == 0 then
                switchSSRGraph()
            else
                switchToGroup(num)
            end 
        end
	end
	return false
end

local o =
	Def.ActorFrame {
    Name = "notChordDensityGraph", -- it's not the chord density graph
    InitCommand = function(self)
        self:SetUpdateFunction(HighlightUpdaterThing)
    end,
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

-- graph bg
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
		local bar = self:GetParent():GetChild("GraphSeekBar")
        local txt = self:GetParent():GetChild("GraphText")
        local bg = self:GetParent():GetChild("GraphTextBG")
        if isOver(self) then
            local mx = INPUTFILTER:GetMouseX()
            local ypos = INPUTFILTER:GetMouseY() - self:GetParent():GetY() + 80
            
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
            local index = convertPercentToIndexForMods(perc)

            local modsToValues = {}
            local modText = ""
            local modNames = {}
            for i, mod in pairs(CalcPatternMod) do
                local mod = mod:gsub("CalcPatternMod_", "")
                for h = 1, 2 do
                    local blah = "L"
                    if h == 2 then
                        blah = "R"
                    end
                    modsToValues[#modsToValues + 1] = graphVecs[mod][h]
                    modNames[#modNames + 1] = mod..blah
                end
            end
            -- add stammod
            for h = 1, 2 do
                local blah = "L"
                    if h == 2 then
                        blah = "R"
                    end
                modsToValues[#modsToValues + 1] = graphVecs["StamMod"][h]
                modNames[#modNames + 1] = "StamMod"..blah
            end
            
            for k, v in pairs(modsToValues) do
                local txt = string.format(modNames[k]..": %5.4f\n", v[index])
                modText = modText .. txt
            end

            modText = modText:sub(1, #modText-1) -- remove the end whitespace
            txt:settext(modText)
		else
			bar:visible(false)
            txt:visible(false)
            bg:visible(false)
		end
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
            
            local index = convertPercentToIndexForMods(perc)
            local modsToValues = {}
            local modText = ""
            local modNames = {}
            for i, mod in pairs(CalcDiffValue) do
                local mod = mod:gsub("CalcDiffValue_", "")
                for h = 1, 2 do
                    local blah = "L"
                    if h == 2 then
                        blah = "R"
                    end
                    modsToValues[#modsToValues + 1] = graphVecs[mod][h]
                    modNames[#modNames + 1] = mod..blah
                end
            end
            
            -- add ptloss
            for h = 1, 2 do
                local blah = "L"
                    if h == 2 then
                        blah = "R"
                    end
                    modsToValues[#modsToValues + 1] = graphVecs["PtLoss"][h]
                    modNames[#modNames + 1] = "PtLoss"..blah
            end

            for k, v in pairs(modsToValues) do
                local txt = string.format(modNames[k]..": %5.4f\n", v[index])
                modText = modText .. txt
            end
            
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

            modText = modText:sub(1, #modText-1) -- remove the end whitespace
            --txt:settext(modText)
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
    put these as the argument for DootSpooks
    1 = One hand jump downscaler
    2 = Anchor downscaler
    3 = Roll downscaler
    4 = Handstream downscaler
    5 = Jumpstream downscaler
    anything else = no output
]]
local modnames = {
    "ohjl",
    "ohjr",
    "anchl",
    "anchr",
    "rolll",
    "rollr",
    "hsl",
    "hsr",
    "jsl",
    "jsr",
    "cjl",
    "cjr",
    "stl",
    "str",
    "ohtl",
    "ohtr",
    "cl",
    "cr",
    "fcl",
    "fcr",
    "wrrl",
    "wrrr",
    "sl",
    "sr",
    
}

local modColors = {
    color("1,0,0"),         -- red          = ohjump left
    color("0.8,0.2,0.2"),   -- light red         (right)
    color("0,0,1"),         -- blue         = anchor left
    color("0.2,0.2,0.8"),   -- light blue        (right)
    color("0,1,0"),         -- green        = roll left
    color("0.3,0.9,0.3"),   -- light green       (right)
    color("1,1,0"),         -- yellow       = handstream left
    color("0.6,0.6,0"),     -- dark yellow      (right)
    color("1,0,1"),         -- purple       = jumpstream left
    color("1,0.3,1"),        -- light purple      (right)
    color("1.4,1.3,1"),       
    color("1.4,1.3,0.9"),
    color(".4,1.3,1"),       
    color(".4,1.3,0.9"),
    color(".7,1.3,1"),       
    color(".7,1.3,0.9"),
    color(".4,0.9,0.3"),       
    color(".4,0.9,0.3"),
    color(".4,0.5,0.59"),       
    color(".4,0.3,0.49"),
    color("1,0,0"),
    color("1,0,0"),
    color("1,0,0"),
    color("1,0,0"),
}

-- top graph average text
makeskillsetlabeltext = function(i, mod, hand)
    return LoadFont("Common Normal") .. {
        Name = "SSLabel"..i,
        InitCommand = function(self)
            local xspace = 55   -- this is gonna look like shit on 4:3 no matter what so w.e
            self:xy(-plotWidth/2 + 5 + math.floor((i-1)/4) * xspace, plotHeight/3.3 + ((i-1)%4)*8.5):halign(0)
            self:zoom(0.3)
            self:settext("")
            self:maxwidth((plotWidth-10) / 0.5)
        end,
        UpdateAveragesMessageCommand = function(self)
            if song then
                local aves = {}
                local values = graphVecs[mod][hand]
                if not values or not values[1] then 
                    self:settext("")
                    return
                end
                for i = 1, #values do
                    if values[i] and #values > 0 then
                        aves[i] = table.average(values)
                    end
                end
                if activeModGroup == -1 or (debugGroups[activeModGroup] and debugGroups[activeModGroup][mod]) then
                    self:diffuse(modColors[i])
                else
                    self:diffuse(color(".3,.3,.3"))
                end
                self:settextf("%s: %.4f", modnames[i], aves[i])
            end
        end,
        UpdateActiveModsMessageCommand = function(self)
            -- if this group is selected and we want to show it off
            if activeModGroup == -1 or (debugGroups[activeModGroup] and debugGroups[activeModGroup][mod]) then
                self:playcommand("UpdateAverages")
            else
                -- grey out unselected groups
                self:diffuse(color(".3,.3,.3"))
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
                    for i = 1, #graphVecs["Jump"][1] do
                        local x = fitX(i, #graphVecs["Jump"][1])
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
                self:diffusealpha(0)
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
            if song and enabled then
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
    color("#b0cec2"),    -- tech
    color("1,0,0"),   
    color("1,0,0"),
}

-- pattern mod lines
for i, mod in pairs(CalcPatternMod) do
    local modname = mod:gsub("CalcPatternMod_", "") -- by design
    o[#o+1] = topGraphLine(modname, modColors[(i * 2) - 1], 1)
    o[#o+1] = topGraphLine(modname, modColors[i * 2], 2)
    o[#o+1] = makeskillsetlabeltext((i * 2) - 1, modname, 1)
    o[#o+1] = makeskillsetlabeltext((i * 2), modname, 2)
end
-- add stam since it's not technically a pattern mod
o[#o+1] = topGraphLine("StamMod", modColors[(#CalcPatternMod * 2) + 1], 1)
o[#o+1] = topGraphLine("StamMod", modColors[(#CalcPatternMod * 2) + 2], 2)
o[#o+1] = makeskillsetlabeltext((#CalcPatternMod * 2) + 1, "StamMod", 1)
o[#o+1] = makeskillsetlabeltext((#CalcPatternMod * 2) + 2, "StamMod", 2)
o[#o+1] = topGraphLine("base_line", modColors[14])    -- super hack to make 1.0 value indicator line

-- MSD mod lines
for i, mod in pairs(CalcDiffValue) do
    local modname = mod:gsub("CalcDiffValue_", "") -- by design
    if i == 2 or i == 4 then   -- these are the most interesting ones atm
        o[#o+1] = bottomGraphLineMSD(modname, skillsetColors[(i * 2) - 1], 1)
        o[#o+1] = bottomGraphLineMSD(modname, skillsetColors[i * 2], 2)
    end
end
o[#o+1] = bottomGraphLineMSD("PtLoss", skillsetColors[(#CalcDiffValue * 2) + 1], 1)
o[#o+1] = bottomGraphLineMSD("PtLoss", skillsetColors[(#CalcDiffValue * 2) + 2], 2)

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
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("0,0,0,.4")):zoomto(20,20)
    end
}

o[#o + 1] = LoadFont("Common Normal") .. {
    Name = "GraphText",
    InitCommand = function(self)
        self:y(8 + plotHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.4)
    end
}

o[#o + 1] = Def.Quad {
    Name = "GraphSeekBar",
    InitCommand = function(self)
        self:zoomto(1, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end
}


return o