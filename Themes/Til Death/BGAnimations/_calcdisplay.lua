local oldWidth = capWideScale(280, 300)
local plotWidth, plotHeight = capWideScale(300,450), 160
local plotX, plotY = oldWidth+3 + plotWidth/2, -20 + plotHeight/2
local bgalpha = 0.9
local enabled = false
local song
local steps

-- bg actors for mouse hover stuff
local topgraph = nil
local bottomgraph = nil

CALC:initdebugstate()

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
    if y < CALC.debugstate.upperGraphMin then y = CALC.debugstate.upperGraphMin + 0.005 end
    local num = scale(y, CALC.debugstate.upperGraphMin, CALC.debugstate.upperGraphMax, 0, 1)
    local out = -1 * num * plotHeight
    return out
end

-- scale values to vertical positions within the lower graph
local function fitY2(y, lb, ub)
    if lb == nil then lb = CALC.debugstate.lowest end
    if ub == nil then ub = CALC.debugstate.highest end
    local num = scale(y, lb, ub, 0, 1)
    local out = -1 * num * plotHeight + plotHeight/2
    return out
end

-- restrict the x bounds of the graph to a specific range of percentages
local function getGraphBounds(vec)
    -- if these vectors are empty, dont restrict
    if vec == nil or #vec[1] == 0 then
        return 0, 1
    else
        -- get the x position of the first and last item, normalize the position, and then convert them to percentages [0,1]
        return (
            fitX(
                CALC.debugstate.firstSecond  / getCurRateValue(),
                CALC.debugstate.finalSecond / getCurRateValue()
            ) + plotWidth/2
        ) / plotWidth,
        (
            fitX(
                #vec[1] + CALC.debugstate.firstSecond,
                CALC.debugstate.finalSecond / getCurRateValue()
            ) + plotWidth/2
        ) / plotWidth
    end
end

-- restrict the x bounds of the graph to a specific range of percentages
local function getGraphBoundsJack(vec)
    -- if these vectors are empty, dont restrict
    if vec == nil or #vec == 0 then
        return 0, 1
    else
        -- get the x position of the first and last item, normalize the position, and then convert them to percentages [0,1]
        return (
            fitX(
                vec[1][1],
                CALC.debugstate.finalSecond/2 / getCurRateValue()
            ) + plotWidth/2
        ) / plotWidth,
        (
            fitX(
                vec[#vec][1],
                CALC.debugstate.finalSecond/2 / getCurRateValue()
            ) + plotWidth/2
        ) / plotWidth
    end
end



-- graph percentage to index (dependent on mod/song length)
local function convertPercentToIndexForMods(leftX, rightX)
    local percent = leftX / rightX
    local lower, upper = getGraphBounds(CALC.debugstate.graphVecs["JS"])

    -- if the percentage given is outside the desired bounds, restrict it to sane bounds
    if percent < lower then
        return CALC:convertPercentToIndexForGraphVec(0)
    elseif percent > upper then
        return CALC:convertPercentToIndexForGraphVec(1)
    else
        -- otherwise scale the number to a percentage .. of a percentage
        percent = scale(percent, lower, upper, 0, 1)
        return CALC:convertPercentToIndexForGraphVec(percent)
    end
end

-- graph percentage to index (dependent on nerv/jack length)
local function convertPercentToIndexForJack(leftX, rightX, vec)
    local percent = leftX / rightX
    local lower, upper = getGraphBoundsJack(vec)

    local finalIndex = 1
    -- if the percentage given is outside the desired bounds, restrict it to sane bounds
    if percent < lower then
        finalIndex = 1
    elseif percent > upper then
        finalIndex = #vec
    else
        local timepercent = percent
        -- otherwise scale the number to a percentage .. of a percentage
        percent = scale(percent, lower, upper, 0, 1)
        
        -- heres the time value of the index we want to end up with but its REALLY likely that wont happen
        local intendedtime = CALC.debugstate.finalSecond/2/getCurRateValue() * timepercent
        -- but we have to binary search the values to find the true index
        -- HAHAHAHAHAHA THIS IS SO BAD BUT IM TRYING TO MAKE IT NOT SO BAD
        local searchpoint = notShit.round(percent * #vec)
        if searchpoint < 1 then searchpoint = 1 end
        if searchpoint > #vec then searchpoint = #vec end
        local lastsearchpoint = 0
        local lastlastsearchpoint = -1 -- checking this to prevent loops
        local lb, ub = 0, #vec
        while lastlastsearchpoint ~= searchpoint do
            if vec[searchpoint][1] < intendedtime then
                lb = searchpoint
                lastlastsearchpoint = lastsearchpoint
                lastsearchpoint = searchpoint
                searchpoint = notShit.round((ub-lb)/2 + lb)
            elseif vec[searchpoint][1] > intendedtime then
                ub = searchpoint
                lastlastsearchpoint = lastsearchpoint
                lastsearchpoint = searchpoint
                searchpoint = notShit.round((ub-lb)/2 + lb)
            else
                -- well this would be exceptionally rare to match a float...
                return searchpoint
            end
        end

        finalIndex = searchpoint
    end
    return finalIndex
end

-- transforms the position of the mouse from the cd graph to the calc info graph
local function transformPosition(pos, w, px)
    local distanceAcrossOriginal = (pos - px) / w
    out = distanceAcrossOriginal * plotWidth - plotWidth/2
    return out
end

-- responsible for updating all relevant values and then triggering the display message(s)
local function updateCoolStuff()
    song = GAMESTATE:GetCurrentSong()
    steps = GAMESTATE:GetCurrentSteps()

    CALC:loadSongLengthInfo(song, steps)
    CALC:preinitDebugValPhase()
    CALC:loadDebugDataForSteps(steps)
    CALC:announceActiveDebugMods()    
end

-- input handler
-- note: returning true in a handler stops input from being sent anywhere else
local function yetAnotherInputCallback(event)
    if event.type == "InputEventType_FirstPress" then
        if event.DeviceInput.button == "DeviceButton_mousewheel up" then
            if isOver(topgraph) then
                CALC:addToModGroup(1)
                return true
            elseif isOver(bottomgraph) then
                CALC:addToDiffGroup(1, steps)
                return true
            end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
            if isOver(topgraph) then
                CALC:addToModGroup(-1)
                return true
            elseif isOver(bottomgraph) then
                CALC:addToDiffGroup(-1, steps)
                return true
            end
        end
	end
	return false
end

local o = Def.ActorFrame {
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
    CurrentStepsChangedMessageCommand = function(self)
        if not enabled then return end
        updateCoolStuff()
        self:RunCommandsOnChildren(
            function(self)
                self:playcommand("DoTheThing")
            end
        )
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("CurrentStepsChanged")
    end,
    Def.Quad {
        Name = "GraphPos",
        InitCommand = function(self)
            self:xy(-plotWidth/2, -20)
            self:zoomto(0, 0)
                :diffuse(color("1,1,1,1"))
                :halign(0)
                :draworder(1100)
                :halign(0)
                :diffusealpha(0.1)
        end,
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(yetAnotherInputCallback)
        end
    }
}

o[#o + 1] = Def.Quad {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight)
            :diffuse(color("#232323"))
            :diffusealpha(bgalpha)
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
            txt:y(ypos - 20)

            local index = convertPercentToIndexForMods(mx - leftEnd, rightEnd - leftEnd)
            txt:settext(CALC.debugstate.debugstrings[index])
		else
            txt:visible(false)
		end
	end
}

-- graph bg
o[#o + 1] = UIElements.QuadButton(1, 1) .. {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight)
            :diffuse(color("#232323"))
            :diffusealpha(bgalpha)
        topgraph = self
    end,
    DoTheThingCommand = function(self)
        local visible = song ~= nil
        self:visible(visible)
        self:z(visible and 5 or -5) -- higher button z has priority (to block musicwheel button clicking)
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

            local index = convertPercentToIndexForMods(mx - leftEnd, rightEnd - leftEnd)
            local modText = CALC:getDebugModsForIndex(
                CalcPatternMod,
                "CalcPatternMod",
                CALC.miscToUpperMods,
                index,
                true
            )
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
        self:xy(-plotWidth/2 + 2,-plotHeight/2 + 5)
        self:halign(0)
        self:zoom(0.25)
        self:settextf("Group %d", CALC.debugstate.activeModGroup)
    end,
    UpdateActiveModsMessageCommand = function(self)
        self:settextf("Group %d", CALC.debugstate.activeModGroup)
    end
}

-- second bg
o[#o + 1] = UIElements.QuadButton(1, 1) .. {
    Name = "G2BG",
    InitCommand = function(self)
        self:y(plotHeight + 5)
        self:zoomto(plotWidth, plotHeight)
            :diffuse(color("#232323"))
            :diffusealpha(bgalpha)
        bottomgraph = self
    end,
    DoTheThingCommand = function(self)
        local visible = song ~= nil
        self:visible(visible)
        self:z(visible and 5 or -5) -- higher button z has priority (to block musicwheel button clicking)
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
            
            if not CALC.diffGroups[CALC.debugstate.activeDiffGroup]["SSRS"] then
                local index = convertPercentToIndexForMods(mx - leftEnd, rightEnd - leftEnd)
                local modText = CALC:getDebugModsForIndex(
                    CalcDiffValue,
                    "CalcDiffValue",
                    CALC.miscToLowerMods,
                    index,
                    false
                )

                if CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"] then
                    modText = modText .. "\n"
                    local jktxt = ""
                    local jkstmtxt = ""
                    local jklosstxt = ""
                    for h = 1,2 do
                        local hnd = h == 1 and "Left" or "Right"
                        if CALC.debugstate.jackdiffs[hnd] ~= nil and #CALC.debugstate.jackdiffs[hnd] > 0 then
                            local hand = h == 1 and "L" or "R"
                            local index = convertPercentToIndexForJack(mx - leftEnd, rightEnd - leftEnd, CALC.debugstate.jackdiffs[hnd])
                            jktxt = jktxt .. string.format("%s: %5.4f\n", "Jack"..hand, CALC.debugstate.jackdiffs[hnd][index][2])
                            jkstmtxt = jkstmtxt .. string.format("%s: %5.4f\n", "Jack Stam"..hand, CALC.debugstate.jackdiffs[hnd][index][3])
                            jklosstxt = jklosstxt .. string.format("%s: %5.4f\n", "Jack Loss"..hand, CALC.debugstate.jackdiffs[hnd][index][4])
                        end
                    end
                    modText = modText .. jktxt .. jkstmtxt .. jklosstxt
                    modText = modText:sub(1, #modText-1) -- remove the end whitespace
                end

                if CALC.diffGroups[CALC.debugstate.activeDiffGroup]["CV"] then
                    modText = modText .. "\n"
                    for h = 1,2 do
                        for c = 1,2 do
                            local hnd = h == 1 and "Left" or "Right"
                            local cl = c == 1 and "Left" or "Right"
                            local hand = h == 1 and "L" or "R"
                            local col = c == 1 and "L" or "R"
                            if CALC.debugstate.cvvals[hnd][cl] ~= nil and #CALC.debugstate.cvvals[hnd][cl] > 0 then
                                local index = convertPercentToIndexForJack(mx - leftEnd, rightEnd - leftEnd, CALC.debugstate.cvvals[hnd][cl])
                                modText = modText .. string.format("%s : %5.4f\n", "CV-"..hand..col, CALC.debugstate.cvvals[hnd][cl][index][2])
                            end
                        end
                    end
                    modText = modText:sub(1, #modText-1) -- remove the end whitespace
                end

                for t = 1,3 do
                    local strs = {"Pewp", "Obliosis", "c"}
                    if CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Tech" .. t] then
                        modText = modText .. "\n"
                        for h = 1,2 do
                            local hnd = h == 1 and "Left" or "Right"
                            local hand = h == 1 and "L" or "R"
                            if CALC.debugstate.techvals[hnd] ~= nil and #CALC.debugstate.techvals[hnd] > 0 then
                                local index = convertPercentToIndexForJack(mx - leftEnd, rightEnd - leftEnd, CALC.debugstate.techvals[hnd])
                                modText = modText .. string.format("%s : %5.4f\n", strs[t]..hand, CALC.debugstate.techvals[hnd][index][t+1])
                            end
                        end
                    end
                end

                txt:settext(modText)
            elseif CALC.diffGroups[CALC.debugstate.activeDiffGroup]["SSRS"] then
                local ssrindex = CALC:convertPercentToIndexForSSRS(perc)
                -- The names here are made under the assumption the skillsets and their positions never change
                local ssrAtIndex = {
                    CALC.debugstate.ssrs[1][ssrindex], -- overall
                    CALC.debugstate.ssrs[2][ssrindex], -- stream
                    CALC.debugstate.ssrs[3][ssrindex], -- jumpstream
                    CALC.debugstate.ssrs[4][ssrindex], -- handstream
                    CALC.debugstate.ssrs[5][ssrindex], -- stamina
                    CALC.debugstate.ssrs[6][ssrindex], -- jackspeed
                    CALC.debugstate.ssrs[7][ssrindex], -- chordjack
                    CALC.debugstate.ssrs[8][ssrindex], -- technical
                }
                local ssrtext = string.format("Percent: %5.4f\n", (CALC.ssrLowerBoundWife + (CALC.ssrUpperBoundWife-CALC.ssrLowerBoundWife)*perc)*100)
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

-- mod group indicators appears top left of top graph bg
o[#o+1] = LoadFont("Common Normal") .. {
    Name = "G2Group",
    InitCommand = function(self)
        self:xy(-plotWidth/2 + 2, plotHeight/2 + 12)
        self:halign(0)
        self:zoom(0.25)
        self:settextf("Group %d", CALC.debugstate.activeDiffGroup)
    end,
    UpdateActiveLowerGraphMessageCommand = function(self)
        self:settextf("Group %d", CALC.debugstate.activeDiffGroup)
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

                local shortname = CALC.modToShortname[mod] .. (hand == 1 and "l" or "r")
                local modcolor = CALC.modToColor[mod]

                local ave
                local values = CALC.debugstate.graphVecs[mod][hand]
                if not values or not values[1] then 
                    self:settext("")
                    return
                end
                if values[i] and #values > 0 then
                    ave = table.average(values)
                end
                if CALC.debugstate.activeModGroup == -1 or (CALC.debugGroups[CALC.debugstate.activeModGroup] and CALC.debugGroups[CALC.debugstate.activeModGroup][mod]) then
                    self:diffuse(modcolor)
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
            if CALC.debugstate.activeModGroup == -1 or (CALC.debugGroups[CALC.debugstate.activeModGroup] and CALC.debugGroups[CALC.debugstate.activeModGroup][mod]) then
                self:playcommand("UpdateAverages", {mods = params.mods})
            else
                -- hide unselected groups
                self:diffusealpha(0)
            end
        end
    }
end

-- lower graph average text
o[#o + 1] = LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:xy(-plotWidth/2 + 30, plotHeight/2 + 12):halign(0)
        self:maxwidth((plotWidth-30) / 0.35)
        self:zoom(0.35)
        self:settext("")
    end,
    SetCommand = function(self)
        if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["SSRS"]) then
            self:settextf("Upper SSR: %.4f", math.max(unpack(CALC.debugstate.ssrs[1])))
        else
            if CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"] and steps then
                local jackpbm = 1.0013144
                local tappoints = steps:GetRelevantRadars()[1] * 2
                local maxpoints = tappoints * jackpbm
                local afterloss = maxpoints - CALC.debugstate.jackLossSumRight - CALC.debugstate.jackLossSumLeft
                local reqpoints = tappoints * 0.93
                self:settextf("Upper Bound: %.2f  |  Loss Sum L: %5.2f  |  Loss Sum R: %5.2f  |  Pt AfterLoss/Req/Max: %5.2f/%5.2f/%5.2f",
                    CALC.debugstate.lowerGraphMaxJack*0.9,
                    CALC.debugstate.jackLossSumLeft,
                    CALC.debugstate.jackLossSumRight,
                    afterloss,
                    reqpoints,
                    maxpoints
                )
            elseif CALC.diffGroups[CALC.debugstate.activeDiffGroup]["CV"] and steps then
                self:settextf("Upper Bound: %.2f  |  Lower Bound: %.2f  |  Average CVs -  LL = %5.2f | LR = %5.2f | RL = %5.2f | RR = %5.2f",
                    CALC.debugstate.cvmax,
                    CALC.debugstate.cvmin,
                    CALC.debugstate.cva[1],
                    CALC.debugstate.cva[2],
                    CALC.debugstate.cva[3],
                    CALC.debugstate.cva[4]
                )
            elseif (CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Tech1"] or CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Tech2"] or CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Tech3"]) and steps then
                self:settextf("Upper Bound: %.2f  |  Lower Bound: %.2f  |  Avg pewpL = %5.2f pewpR = %5.2f  |  Avg oblioL = %5.2f oblioR = %5.2f  | Avg cL = %5.2f cR = %5.2f", 
                    math.max(
                        CALC.debugstate.techminmaxavg["Left"][1][2],
                        CALC.debugstate.techminmaxavg["Left"][2][2],
                        CALC.debugstate.techminmaxavg["Right"][1][2],
                        CALC.debugstate.techminmaxavg["Right"][2][2]
                    ),
                    math.min(
                        CALC.debugstate.techminmaxavg["Left"][1][1],
                        CALC.debugstate.techminmaxavg["Left"][2][1],
                        CALC.debugstate.techminmaxavg["Left"][3][1],
                        CALC.debugstate.techminmaxavg["Right"][1][1],
                        CALC.debugstate.techminmaxavg["Right"][2][1],
                        CALC.debugstate.techminmaxavg["Right"][3][1]
                    ),
                    CALC.debugstate.techminmaxavg["Left"][1][3],
                    CALC.debugstate.techminmaxavg["Right"][1][3],
                    CALC.debugstate.techminmaxavg["Left"][2][3],
                    CALC.debugstate.techminmaxavg["Right"][2][3],
                    CALC.debugstate.techminmaxavg["Left"][3][3],
                    CALC.debugstate.techminmaxavg["Right"][3][3]
                )
            else
                self:settextf("Upper Bound: %.4f  |  Grindscaler: %5.2f",
                    CALC.debugstate.lowerGraphMax,
                    CALC.debugstate.grindscaler
                )
            end
        end
    end,
    DoTheThingCommand = function(self)
        self:playcommand("Set")
    end,
    UpdateActiveLowerGraphMessageCommand = function(self)
        self:playcommand("Set")
    end
}

local dotWidth = 0
local function setOffsetVerts(vt, x, y, c)
	vt[#vt + 1] = {{x - dotWidth, y + dotWidth, 0}, c}
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
                    for i = 1, #CALC.debugstate.graphVecs["JS"][1] do
                        local x = fitX(i, #CALC.debugstate.graphVecs["JS"][1])
                        local y = fitY1(1)
                        y = y + plotHeight / 2
                        setOffsetVerts(verts, x, y, color("1,1,1"))
                    end
                    if #verts <= 1 then
                        verts = {}
                    end
                    self:SetVertices(verts)
                    self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
                    return
                end

                if not CALC.debugstate.graphVecs[mod] then return end
                local values = CALC.debugstate.graphVecs[mod][hand]
                if not values or not values[1] then return end
                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    local x = fitX(i + CALC.debugstate.firstSecond / getCurRateValue() - 1, CALC.debugstate.finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY1(values[i])
                    y = y + plotHeight / 2
                    setOffsetVerts(verts, x, y, colorToUse) 
                end

                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}

                if CALC.debugstate.activeModGroup == -1 or (CALC.debugGroups[CALC.debugstate.activeModGroup] and CALC.debugGroups[CALC.debugstate.activeModGroup][mod]) then
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
            if CALC.debugstate.activeModGroup == -1 or (CALC.debugGroups[CALC.debugstate.activeModGroup] and CALC.debugGroups[CALC.debugstate.activeModGroup][mod]) then
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

local function topGraphLineJackStam(mod, colorToUse, hand)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = CALC.debugstate.jackdiffs[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], CALC.debugstate.finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY1(values[i][3]) + plotHeight/2

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
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
                
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup][mod]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local verts = {}
                if not CALC.debugstate.graphVecs[mod] then return end
                local values = CALC.debugstate.graphVecs[mod][hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    local x = fitX(i + CALC.debugstate.firstSecond  / getCurRateValue() - 1, CALC.debugstate.finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i], CALC.debugstate.lowerGraphMin, CALC.debugstate.lowerGraphMax)

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup][mod]) then
                    self:visible(true)
                else
                    self:visible(false)
                end
            end
        end
    }
end

local function bottomGraphLineJack(colorToUse, hand)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = CALC.debugstate.jackdiffs[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], CALC.debugstate.finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][2], CALC.debugstate.lowerGraphMin, CALC.debugstate.lowerGraphMaxJack)

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end
            end
        end
    }
end

local function bottomGraphLineJackloss(colorToUse, hand)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = CALC.debugstate.jackdiffs[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], CALC.debugstate.finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][4], CALC.debugstate.lowerGraphMin, CALC.debugstate.lowerGraphMax)

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end
            end
        end
    }
end

local function bottomGraphLineCoeffVariance(colorToUse, hand, col)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["CV"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local col = col == 1 and "Left" or "Right"
                local verts = {}
                local values = CALC.debugstate.cvvals[hand][col]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], CALC.debugstate.finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][2], CALC.debugstate.cvmin - 0.1, CALC.debugstate.cvmax + 0.25)

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["CV"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end
            end
        end
    }
end

local function bottomGraphLineTechVal(colorToUse, hand, techValIndex)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Tech" .. techValIndex]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = CALC.debugstate.techvals[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], CALC.debugstate.finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][1+techValIndex], CALC.debugstate.techminmaxavg[hand][techValIndex][1], CALC.debugstate.techminmaxavg[hand][techValIndex][2] * 1.20)

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["Tech" .. techValIndex]) then
                    self:visible(true)
                else
                    self:visible(false)
                end
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
            if song and enabled and #CALC.debugstate.ssrs > 0 then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["SSRS"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local verts = {}

                for i = 1, #CALC.debugstate.ssrs[lineNum] do
                    local x = fitX(i, #CALC.debugstate.ssrs[lineNum]) -- vector length based positioning
                    --local x = fitX(i + firstSecond  / getCurRateValue() - 1, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY2(CALC.debugstate.ssrs[lineNum][i])

                    setOffsetVerts(verts, x, y, colorToUse)
                end
                
                if #verts <= 1 then
                    verts = {}
                end
                self:SetVertices(verts)
                self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #verts}
            else
                self:visible(false)
            end
        end,
        UpdateActiveLowerGraphMessageCommand = function(self)
            if song and enabled then
                if CALC.debugstate.activeDiffGroup == -1 or (CALC.diffGroups[CALC.debugstate.activeDiffGroup] and CALC.diffGroups[CALC.debugstate.activeDiffGroup]["SSRS"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end
            end
        end,
        UpdateSSRLinesMessageCommand = function(self)
            self:playcommand("DoTheThing")
        end
    }
end

-- upper mod lines and text
-- we do the hand loop inner so the text lines up with hands next to each other
do -- scoping
    local i
    for i, mod in pairs(CalcPatternMod) do
        for h = 1,2 do
            local modname = shortenEnum("CalcPatternMod", mod)
            o[#o+1] = topGraphLine(modname, CALC.modToColor[modname], h)
        end
    end
    i = 1
    for mod, _ in pairs(CALC.miscToUpperMods) do
        for h = 1,2 do
            -- dont have to shorten enum here because i did something dumb
            o[#o+1] = topGraphLine(mod, CALC.modToColor[mod], h)
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
            o[#o+1] = bottomGraphLineMSD(modname, CALC.modToColor[modname], h)
        end
    end
    i = 1
    for mod, _ in pairs(CALC.miscToLowerMods) do
        for h = 1,2 do
            o[#o+1] = bottomGraphLineMSD(mod, CALC.modToColor[mod], h)
        end
        i = i + 1
    end
end

-- SSR skillset lines
for i = 1,#CALC.skillsetColors do
    o[#o+1] = bottomGraphLineSSR(i, CALC.skillsetColors[i])
end

-- Jack diff line(s)
for h = 1,2 do
    local colr = CALC.jackdiffColors[h]
    o[#o+1] = bottomGraphLineJack(colr, h)
end

-- jack stam
for h = 1,2 do
    o[#o+1] = topGraphLineJackStam("jack_stam", color("1,1,1,1"), h)
end

-- jack loss
for h = 1,2 do
    local colr = CALC.jackdiffColors[h+2]
    o[#o+1] = bottomGraphLineJackloss(colr, h)
end

-- cv vals
do
    local i = 1
    for h = 1,2 do
        for c = 1,2 do
            local colr = CALC.cvColors[i]
            i = i + 1
            o[#o+1] = bottomGraphLineCoeffVariance(colr, h, c)
        end
    end
end

-- tech vals
do
    local i = 1
    for h = 1,2 do
        for techvalindex = 1,3 do
            local colr = CALC.techColors[i]
            i = i + 1
            o[#o+1] = bottomGraphLineTechVal(colr, h, techvalindex)
        end
    end
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
        self:y(8 + plotHeight+5):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.5):maxheight(500)
    end
}

o[#o + 1] = Def.Quad {
    Name = "GraphSeekBar",
    InitCommand = function(self)
        self:zoomto(1, plotHeight):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
    end
}


return o
