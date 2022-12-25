local oldWidth = capWideScale(280, 300)
local plotWidth, plotHeight = capWideScale(300,450), 160
local plotX, plotY = oldWidth+3 + plotWidth/2, -20 + plotHeight/2
local highest = 0
local lowest = 0
local lowerGraphMax = 0
local lowerGraphMaxJack = 0
local jackLossSumLeft = 0
local jackLossSumRight = 0
local upperGraphMax = 0
local lowerGraphMin = 0
local upperGraphMin = 0
local bgalpha = 0.9
local enabled = false
local song
local steps

-- used for positioning graphs by time instead of vector length
local finalSecond = 0
local firstSecond = 0
local steplength = 0

local graphVecs = {}
local jackdiffs = {}
local cvvals = {}
local cva = {}
local cvmax = 1
local cvmin = 0
local ssrs = {}
local grindscaler = 0
local activeModGroup = 1
local activeDiffGroup = 1
local debugstrings
local techvals = {}
local techminmaxavg = {}

-- bg actors for mouse hover stuff
local topgraph = nil
local bottomgraph = nil

local function calcCVA()
    cva = {0,0,0,0}
    cvmax = -1
    cvmin = 999
    if #cvvals["Left"]["Left"] == 0 and #cvvals["Left"]["Right"] == 0 and #cvvals["Right"]["Left"] == 0 and #cvvals["Right"]["Right"] == 0 then
        return
    end
    local i = 1
    for h, hand in pairs(cvvals) do
        for c, col in pairs(hand) do
            local sum = 0
            for _, v in ipairs(col) do
                if v[2] < cvmin then cvmin = v[2] end
                if v[2] > cvmax then cvmax = v[2] end
                sum = sum + v[2]
            end
            cva[i] = sum / #col
            i = i + 1
        end
    end
end

local function calcTVA()
    techminmaxavg = {Left = {{999,0,0},{999,0,0},{999,0,0}}, Right = {{999,0,0},{999,0,0},{999,0,0}}}
    if #techvals["Left"] == 0 and #techvals["Right"] == 0 then
        return
    end
    for h, hand in pairs(techvals) do
        local sum = {0,0,0}
        for _, v in ipairs(hand) do
            if v[2] < techminmaxavg[h][1][1] then techminmaxavg[h][1][1] = v[2] end
            if v[2] > techminmaxavg[h][1][2] then techminmaxavg[h][1][2] = v[2] end
            if v[3] < techminmaxavg[h][2][1] then techminmaxavg[h][2][1] = v[3] end
            if v[3] > techminmaxavg[h][2][2] then techminmaxavg[h][2][2] = v[3] end
            if v[4] < techminmaxavg[h][3][1] then techminmaxavg[h][3][1] = v[4] end
            if v[4] > techminmaxavg[h][3][2] then techminmaxavg[h][3][2] = v[4] end
            sum[1] = sum[1] + v[2]
            sum[2] = sum[2] + v[3]
            sum[3] = sum[3] + v[4]
        end
        techminmaxavg[h][1][3] = sum[1] / #hand
        techminmaxavg[h][2][3] = sum[2] / #hand
        techminmaxavg[h][3][3] = sum[3] / #hand
    end
end

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

-- restrict the x bounds of the graph to a specific range of percentages
local function getGraphBounds(vec)
    -- if these vectors are empty, dont restrict
    if vec == nil or #vec[1] == 0 then
        return 0, 1
    else
        -- get the x position of the first and last item, normalize the position, and then convert them to percentages [0,1]
        return (fitX(firstSecond  / getCurRateValue(), finalSecond / getCurRateValue()) + plotWidth/2) / plotWidth, (fitX(#vec[1] + firstSecond, finalSecond / getCurRateValue()) + plotWidth/2) / plotWidth
    end
end

-- restrict the x bounds of the graph to a specific range of percentages
local function getGraphBoundsJack(vec)
    -- if these vectors are empty, dont restrict
    if vec == nil or #vec == 0 then
        return 0, 1
    else
        -- get the x position of the first and last item, normalize the position, and then convert them to percentages [0,1]
        return (fitX(vec[1][1], finalSecond/2 / getCurRateValue()) + plotWidth/2) / plotWidth, (fitX(vec[#vec][1], finalSecond/2 / getCurRateValue()) + plotWidth/2) / plotWidth
    end
end

-- convert a percentage (distance horizontally across the graph) to an index 
local function _internalConvPercToInd(x, vec, lowerlimit, upperlimit)
    local output = x
    if output < 0 then output = 0 end
    if output > 1 then output = 1 end
    if lowerlimit == nil then lowerlimit = 1 end
    if upperlimit == nil then upperlimit = #vec end

    local ind = notShit.round(output * #vec[1])
    if ind < 1 then ind = 1 end
    return ind
end

-- graph percentage to index (independent of song length)
local function convertPercentToIndex(x)
    return _internalConvPercToInd(x, ssrs)
end

-- graph percentage to index (dependent on mod/song length)
local function convertPercentToIndexForMods(leftX, rightX)
    local percent = leftX / rightX
    local lower, upper = getGraphBounds(graphVecs["JS"])

    -- if the percentage given is outside the desired bounds, restrict it to sane bounds
    if percent < lower then
        return _internalConvPercToInd(0, graphVecs["JS"])
    elseif percent > upper then
        return _internalConvPercToInd(1, graphVecs["JS"])
    else
        -- otherwise scale the number to a percentage .. of a percentage
        percent = scale(percent, lower, upper, 0, 1)
        return _internalConvPercToInd(percent, graphVecs["JS"])
    end
end

-- graph percentage to index (dependent on nerv/jack length)
local function convertPercentToIndexForJack(leftX, rightX, vec)
    local percent = leftX / rightX
    local lower, upper = getGraphBoundsJack(vec)

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
        local intendedtime = finalSecond/2/getCurRateValue() * timepercent
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
    distanceAcrossOriginal = (pos - px) / w
    out = distanceAcrossOriginal * plotWidth - plotWidth/2
    return out
end

-- for SSR graph generator, modify these constants
local ssrLowerBoundWife = 0.90 -- left end of the graph
local ssrUpperBoundWife = 0.97 -- right end of the graph
local ssrResolution = 100 -- higher number = higher resolution graph (and lag)

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

    -- these mods are not really in the CalcDebugMisc enum. they are not real.
    -- if this gets messed up things do not work
    TotalPatternModStream = true,
    TotalPatternModJumpstream = true,
    TotalPatternModHandstream = true,
    TotalPatternModChordjack = true,
    TotalPatternModTechnical = true,
}

-- list of all additional enums to include in the lower graph
-- it is assumed these are members of CalcDebugMisc (they arent)
local miscToLowerMods = {
    Pts = true,
    PtLoss = true,

    -- these mods are not really in the CalcDebugMisc enum. they are not real.
    -- if this gets messed up things do not work
    PtLossStream = true,
    PtLossJumpstream = true,
    PtLossHandstream = true,
    PtLossChordjack = true,
    PtLossTechnical = true,
    MSDStream = true,
    MSDJumpstream = true,
    MSDHandstream = true,
    MSDChordjack = true,
    MSDTechnical = true,
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

-- convolution
local upperExtraExtraMods = {
    TotalPatternModStream = true,
    TotalPatternModJumpstream = true,
    TotalPatternModHandstream = true,
    TotalPatternModChordjack = true,
    TotalPatternModTechnical = true,
}
local orderedExtraExtraUpperMods = {
    "TotalPatternModStream",
    "TotalPatternModJumpstream",
    "TotalPatternModHandstream",
    "TotalPatternModChordjack",
    "TotalPatternModTechnical",
}
local lowerExtraExtraMods = {
    PtLossStream = true,
    PtLossJumpstream = true,
    PtLossHandstream = true,
    PtLossChordjack = true,
    PtLossTechnical = true,
    MSDStream = true,
    MSDJumpstream = true,
    MSDHandstream = true,
    MSDChordjack = true,
    MSDTechnical = true,
}
local orderedExtraExtraLowerMods = {
    "PtLossStream",
    "PtLossJumpstream",
    "PtLossHandstream",
    "PtLossChordjack",
    "PtLossTechnical",
    "MSDStream",
    "MSDJumpstream",
    "MSDHandstream",
    "MSDChordjack",
    "MSDTechnical",
}

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
        RollJS = true,
    },
	{   -- Group 3
        HS = true,
        StamMod = true,
        OHJumpMod = true,
        HSDensity = true,
        RollJS = true,
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
        WideRangeJJ = true,
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
        CJOHAnchor = true,
    },
    {   -- Group 10
        Roll = true,
        RollJS = true,
    },
    {   -- Group 11
        TotalPatternModStream = true,
    },
    {   -- Group 12
        TotalPatternModJumpstream = true,
    },
    {   -- Group 13
        TotalPatternModHandstream = true,
    },
    {   -- Group 14
        TotalPatternModChordjack = true,
    },
    {   -- Group 15
        TotalPatternModTechnical = true,
    },
    {   -- Group 16
        TheThing = true,
        TheThing2 = true,
    },
    {   -- Group 17
        Minijack = true,
    },
}

-- specify enum names here
-- only CalcDiffValue enums
-- also specify SSR to show all SSRs (recommend to leave a group alone)
-- also specify Jack to show the Jack diffs for both hands
-- miscDebugMods that are also specified under miscToLowerMods can be placed here
local diffGroups = {
    {   -- Group 1
        NPSBase = true,
        MSDStream = true,
    },
    {   -- Group 2
        NPSBase = true,
        MSDJumpstream = true,
    },
    {   -- Group 3
        NPSBase = true,
        MSDHandstream = true,
    },
    {   -- Group 4
        NPSBase = true,
        MSDChordjack = true,
    },
    {   -- Group 5
        NPSBase = true,
        MSDTechnical = true,
    },
    {   -- Group 6
        Pts = true,
        PtLossStream = true
    },
    {   -- Group 7
        Pts = true,
        PtLossJumpstream = true
    },
    {   -- Group 8
        Pts = true,
        PtLossHandstream = true
    },
    {   -- Group 9
        Pts = true,
        PtLossChordjack = true
    },
    {   -- Group 10
        Pts = true,
        PtLossTechnical = true
    },
    {   -- Group 11
        Jack = true,
    },
    {   -- Group 12
        JackBase = true,
    },
    {   -- Group 13
        NPSBase = true,
        TechBase = true,
    },
    {   -- Group 14
        RMABase = true,
    },
    {   -- Group 15
        MSBase = true,
        NPSBase = true,
        CJBase = true,
    },
    {   -- Group 16
        CV = true,
    },
    {   -- Group 17
        Tech1 = true, -- "pewp" values
    },
    {   -- Group 18
        Tech2 = true, -- "obliosis" values
    },
    {   -- Group 19
        Tech3 = true, -- "c" values
    },
    {   -- Group 20
        SSRS = true,
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
    steps = GAMESTATE:GetCurrentSteps()
    if song then
        -- account for rate separately
        -- double the output because intervals are half seconds
        firstSecond = steps:GetFirstSecond() * 2
        finalSecond = steps:GetLastSecond() * 2
        steplength = (finalSecond - firstSecond) -- this is "doubled" here
    end
    jackdiffs = {Left = {}, Right = {}}
    cvvals = {Left = {Left = {}, Right = {}}, Right = {Left = {}, Right = {}}}
    techvals = {Left = {}, Right = {}}
    if steps then
        -- Only load SSRs if currently displaying them; this is a major slowdown
        if diffGroups[activeDiffGroup]["SSRS"] then
            ssrs = getGraphForSteps(steps)
        else
            ssrs = {}
        end
        lowerGraphMax = 0
        lowerGraphMaxJack = 0
        jackLossSumLeft = 0
        jackLossSumRight = 0
        local bap = steps:GetCalcDebugOutput() -- this must be called first before GetCalcDebugExt and GetCalcDebugJack
        local pap = steps:GetCalcDebugExt()
        debugstrings = steps:GetDebugStrings()

        grindscaler = bap["Grindscaler"]

        -- Jack debug output got hyper convoluted so im trying to make it as sane as possible
        -- basically jackdiffs[hand][index] = {row time, diff, stam, loss}
        -- this is so we can place the indices based on row time instead of index
        -- also keep in mind the row times are already changed for each rate so 1.1 will be smaller than 1.0
        -- also all the row times are relative to the first non empty noterow so lets just pad it by the firstsecond/2 too
        -- the odd logic below is done because the length of the hand vectors are often different, but the stam and diff vectors are the same
        -- that allows us to combine the two
        -- the reassignment is done based on the longest vector so 2n iterations are not necessary
        -- (in hindsight this is probably exactly the same runtime whatever)
        local jap = steps:GetCalcDebugJack()["JackHand"]
        if jap then
            local upperiter = #jap["Left"] > #jap["Right"] and #jap["Left"] or #jap["Right"]
            for i = 1, upperiter do
                if jap["Left"][i] then
                    jackdiffs["Left"][#jackdiffs["Left"] + 1] = { jap["Left"][i][1] + firstSecond/2/getCurRateValue(), jap["Left"][i][2], jap["Left"][i][3], jap["Left"][i][4] }
                    if jap["Left"][i][2] > lowerGraphMaxJack then lowerGraphMaxJack = jap["Left"][i][2] end
                    jackLossSumLeft = jackLossSumLeft + jap["Left"][i][4]
                end
                if jap["Right"][i] then
                    jackdiffs["Right"][#jackdiffs["Right"] + 1] = { jap["Right"][i][1] + firstSecond/2/getCurRateValue(), jap["Right"][i][2], jap["Right"][i][3], jap["Right"][i][4] }
                    if jap["Right"][i][2] > lowerGraphMaxJack then lowerGraphMaxJack = jap["Right"][i][2] end
                    jackLossSumRight = jackLossSumRight + jap["Right"][i][4]
                end
            end
        end
        -- squeeze graph
        lowerGraphMaxJack = lowerGraphMaxJack / 0.9

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

        -- even more debug output
        local function fc(arr, name, fallbackValue, top)
            for i, ss in ipairs(ms.SkillSets) do
                graphVecs[name..ss] = {}
                for h = 1,2 do
                    local hand = h == 1 and "Left" or "Right"
                    graphVecs[name..ss][h] = {}
                    if arr ~= nil then
                        for j = 1, #arr[hand][i] do
                            local val = arr[hand][i][j]
                            if val ~= val or val == nil or val == math.huge or val == -math.huge then val = fallbackValue end -- get rid of nan and nil
                            if top then
                                if val > upperGraphMax then upperGraphMax = val end
                            else
                                if val > lowerGraphMax then lowerGraphMax = val end
                            end
                            graphVecs[name..ss][h][j] = val
                        end
                    end
                end
            end 
        end

        if pap ~= nil then
            fc(pap["DebugTotalPatternMod"], "TotalPatternMod", 1, true)
            fc(pap["DebugPtLoss"], "PtLoss", 0, false)
            fc(pap["DebugMSD"], "MSD", 0, false)
            for hand, handvals in pairs(pap["DebugMovingWindowCV"]) do
                for col, colvals in pairs(handvals) do
                    for i, vv in ipairs(colvals) do
                        cvvals[hand][col][i] = {vv[1] + firstSecond/2/getCurRateValue(), vv[2]}
                    end
                end
            end
            for hand, handvals in pairs(pap["DebugTechVals"]) do
                for i, vv in ipairs(handvals) do
                    if (vv[2] > 10) then vv[2] = 0 end

                    techvals[hand][i] = {vv[1] + firstSecond/2/getCurRateValue(), vv[2], vv[3], 1 / vv[4]}
                end
            end
        end
        calcCVA()
        calcTVA()

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

--[[
    ... uhhh so this does the same as the immediately above thing
    yeah
]]
local function switchToDiffGroup(num)
    if num == activeDiffGroup then
        -- activeDiffGroup = -1
    else
        activeDiffGroup = num
    end

    -- generate ssrs only if they are visible, but only once
    -- they get cleared on song change
    -- (it lags)
    if diffGroups[activeDiffGroup]["SSRS"] then
        if #ssrs == 0 and steps then
            ssrs = getGraphForSteps(steps)
            MESSAGEMAN:Broadcast("UpdateSSRLines")
        end
    end

    MESSAGEMAN:Broadcast("UpdateActiveLowerGraph")
end

-- move the active diff group value in a direction (looping)
local function addToDiffGroup(direction)
    if activeDiffGroup == -1 then
        if direction < 0 then
            switchToDiffGroup(#diffGroups)
        elseif direction > 0 then
            switchToDiffGroup(1)
        end
    else
        local newg = (((activeDiffGroup) + direction) % (#diffGroups + 1))
        if newg == 0 then
            newg = direction > 0 and 1 or #diffGroups
        end
        switchToDiffGroup(newg)
    end
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
        if isUpper and (activeModGroup == -1 or debugGroups[activeModGroup][namenoHand]) then
            local name = modNames[k] and modNames[k] or ""
            local value = v[index] and v[index] or 0
            local txt = string.format(name..": %5.4f\n", value)
            modText = modText .. txt
        elseif not isUpper and (activeDiffGroup == -1 or diffGroups[activeDiffGroup][namenoHand]) then
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
                addToDiffGroup(1)
                return true
            end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
            if isOver(topgraph) then
                addToModGroup(-1)
                return true
            elseif isOver(bottomgraph) then
                addToDiffGroup(-1)
                return true
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
            txt:y(ypos - 20)

            local index = convertPercentToIndexForMods(mx - leftEnd, rightEnd - leftEnd)
            txt:settext(debugstrings[index])
		else
            txt:visible(false)
		end
	end
}

-- graph bg
o[#o + 1] = UIElements.QuadButton(1, 1) .. {
    InitCommand = function(self)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
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
        self:xy(-plotWidth/2 + 2,-plotHeight/2 + 5)
        self:halign(0)
        self:zoom(0.25)
        self:settextf("Group %d", activeModGroup)
    end,
    UpdateActiveModsMessageCommand = function(self)
        self:settextf("Group %d", activeModGroup)
    end
}

-- second bg
o[#o + 1] = UIElements.QuadButton(1, 1) .. {
    Name = "G2BG",
    InitCommand = function(self)
        self:y(plotHeight + 5)
        self:zoomto(plotWidth, plotHeight):diffuse(color("#232323")):diffusealpha(
            bgalpha
        )
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
            
            if not diffGroups[activeDiffGroup]["SSRS"] then
                local index = convertPercentToIndexForMods(mx - leftEnd, rightEnd - leftEnd)
                local modText = getDebugModsForIndex(CalcDiffValue, "CalcDiffValue", miscToLowerMods, index, false)

                if diffGroups[activeDiffGroup]["Jack"] then
                    modText = modText .. "\n"
                    local jktxt = ""
                    local jkstmtxt = ""
                    local jklosstxt = ""
                    for h = 1,2 do
                        local hnd = h == 1 and "Left" or "Right"
                        if jackdiffs[hnd] ~= nil and #jackdiffs[hnd] > 0 then
                            local hand = h == 1 and "L" or "R"
                            local index = convertPercentToIndexForJack(mx - leftEnd, rightEnd - leftEnd, jackdiffs[hnd])
                            jktxt = jktxt .. string.format("%s: %5.4f\n", "Jack"..hand, jackdiffs[hnd][index][2])
                            jkstmtxt = jkstmtxt .. string.format("%s: %5.4f\n", "Jack Stam"..hand, jackdiffs[hnd][index][3])
                            jklosstxt = jklosstxt .. string.format("%s: %5.4f\n", "Jack Loss"..hand, jackdiffs[hnd][index][4])
                        end
                    end
                    modText = modText .. jktxt .. jkstmtxt .. jklosstxt
                    modText = modText:sub(1, #modText-1) -- remove the end whitespace
                end

                if diffGroups[activeDiffGroup]["CV"] then
                    modText = modText .. "\n"
                    for h = 1,2 do
                        for c = 1,2 do
                            local hnd = h == 1 and "Left" or "Right"
                            local cl = c == 1 and "Left" or "Right"
                            local hand = h == 1 and "L" or "R"
                            local col = c == 1 and "L" or "R"
                            if cvvals[hnd][cl] ~= nil and #cvvals[hnd][cl] > 0 then
                                local index = convertPercentToIndexForJack(mx - leftEnd, rightEnd - leftEnd, cvvals[hnd][cl])
                                modText = modText .. string.format("%s : %5.4f\n", "CV-"..hand..col, cvvals[hnd][cl][index][2])
                            end
                        end
                    end
                    modText = modText:sub(1, #modText-1) -- remove the end whitespace
                end

                for t = 1,3 do
                    local strs = {"Pewp", "Obliosis", "c"}
                    if diffGroups[activeDiffGroup]["Tech" .. t] then
                        modText = modText .. "\n"
                        for h = 1,2 do
                            local hnd = h == 1 and "Left" or "Right"
                            local hand = h == 1 and "L" or "R"
                            if techvals[hnd] ~= nil and #techvals[hnd] > 0 then
                                local index = convertPercentToIndexForJack(mx - leftEnd, rightEnd - leftEnd, techvals[hnd])
                                modText = modText .. string.format("%s : %5.4f\n", strs[t]..hand, techvals[hnd][index][t+1])
                            end
                        end
                    end
                end

                txt:settext(modText)
            elseif diffGroups[activeDiffGroup]["SSRS"] then
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

-- mod group indicators appears top left of top graph bg
o[#o+1] = LoadFont("Common Normal") .. {
    Name = "G2Group",
    InitCommand = function(self)
        self:xy(-plotWidth/2 + 2, plotHeight/2 + 12)
        self:halign(0)
        self:zoom(0.25)
        self:settextf("Group %d", activeDiffGroup)
    end,
    UpdateActiveLowerGraphMessageCommand = function(self)
        self:settextf("Group %d", activeDiffGroup)
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
    "strm",
    "js",
    --"jss",
    --"jsj",
    "hs",
    --"hss",
    --"hsj",
    "cj",
    --"cjs",
    --"cjj",
    "cjd",
    "hsd",
    "cjohanch",
    "ohj",
    --"ohjbp",
    --"ohjpc",
    --"ohjsc",
    --"ohjms",
    --"ohjcct",
    --"ohjht",
    "cjohj",
    --"cjohjpc",
    --"cjohjsc",
    "balnc",
    "roll",
    "rolljs",
    "oht",
    "voht",
    "chaos",
    "flam",
    "wrr",
    "wrjt",
    "wrjj",
    "wrb",
    "wra",
    "thing",
    "thing2",
    "rm",
    "minij",
    --"rl",
    --"ral",
    --"ralm",
    --"rj",
    --"roht",
    --"ros",
    --"rpa",
    --"rpo",
    --"rpoht",
    --"rpos",
    --"rpj",
    "totpm",


    -- CalcPatternMods above this line
    -- CalcDebugMisc mods meant for only the top graph:
    -- (this list should match the miscToUpperMods list)
    "stam",

    -- everything from here below is in the orderedExtraExtraUpperMods table. dont mess it up
    "tpmstr",
    "tpmjs",
    "tpmhs",
    "tpmcj",
    "tpmtech",
}

-- this list has order
-- try to keep it exactly in the order of the enums used :)
local modColors = {
    -- CalcDebugPattern Colors
    color(".3,1.3,1"),      -- cyan			= stream
	color("1,0,1"),     	-- purple       = jumpstream
	--color("0,1,1"),			-- cyan			= jumpstream stream
	--color("1,0,0"),			-- red			= jumpstream jack
    color("0.6,0.6,0"),     -- dark yellow  = handstream
	--color("0,1,1"),			-- cyan			= handstream stream
	--color("1,0,0"),			-- red			= handstream jack
    color("1.4,1.3,1"),     -- white 		= chordjack
	--color("0,1,1"),			-- cyan			= chordjack stream
	--color("1,0,0"),			-- red			= chordjack jack
	color("1,1,0"),			-- yellow		= cjdensity
    color("1,1,0"),         -- yello        = hsdensity
    color(".1,.3,.9"),      -- something    = CJOHAnchor
    color("1,0.4,0"),       -- orange2		= ohjump
	--color("1,1,1"),			-- ohjbp
	--color("1,1,1"),			-- ohjpc
	--color("1,1,1"),			-- ohjsc
	--color("1,1,1"),			-- ohjms
	--color("1,1,1"),			-- ohjcct
	--color("1,1,1"),			-- ohjht
    color("1,0.4,0"),		-- orange2		= cjohj
	--color("1,1,1"),			-- cjohjpc
	--color("1,1,1"),			-- cjohjsc
    color("0.2,0.2,1"),     -- blue         = balance
    color("0,1,0"),         -- green        = roll
    color("0,1,0"),         -- green        = rolljs
    color(".8,1.3,1"),      -- whiteblue	= oht
    color("1,0,1"),         -- purple       = voht
    color(".4,0.9,0.3"),    -- green		= chaos
    color(".4,0.5,0.59"),   -- teal			= flamjam
    color("1,0.2,0"),		-- red			= wrr
    color("1,0.5,0"),		-- orange		= wrjt
    color("1,0.2,1"),		-- purpley		= wrjj
    color("0.7,1,0.2"),		-- leme			= wrb
    color("0.7,1,0.1"),		-- leme			= wra
    color("0,0.8,1"),		-- light blue	= thething
    color("0,0.6,1"),       -- darkish blue = thething2
	color("0.2,1,1"),		-- light blue	= ranman
    color(".8,1.3,1"),      -- whiteblue	= minijack
	--color("1,1,1"),			-- rl
	--color("1,1,1"),			-- ral
	--color("1,1,1"),			-- ralm
	--color("1,1,1"),			-- rj
	--color("1,1,1"),			-- roht
	--color("1,1,1"),			-- ros
	--color("1,1,1"),			-- rpa
	--color("1,1,1"),			-- rpo
	--color("1,1,1"),			-- rpoht
	--color("1,1,1"),			-- rpos
	--color("1,1,1"),			-- rpj
    color("0.7,1,0"),		-- lime			= totalpatternmod


    -- place CalcPatternMod Colors above this line
    -- MISC MODS START HERE (same order as miscToUpperMods)
    color("0.7,1,0"),		-- lime			= stam

    -- everything starting from here downwards are only mods in the orderedExtraExtraUpperMods table
    color("0.7,1,0"),		-- lime			= totalpatternmodstream
    color("0.7,1,0"),		-- lime			= totalpatternmodjs
    color("0.7,1,0"),		-- lime			= totalpatternmodhs
    color("0.7,1,0"),		-- lime			= totalpatternmodcj
    color("0.7,1,0"),		-- lime			= totalpatternmodtech
}

local skillsetColors = {
    color("1,1,1"),     -- overall
    color("#333399"),   -- stream
    color("#6666ff"),   -- jumpstream
    color("#cc33ff"),   -- handstream
    color("#ff99cc"),   -- stamina
    color("#009933"),   -- jack
    color("#66ff66"),   -- chordjack
    color("#808080"),    -- tech
}

local jackdiffColors = {
    color("1,1,0"), -- jack diff left
    color(".6,0,.7"), -- jack diff right
    color("1,0,0,1"), -- jack loss left
    color("1,0,0,1"), -- jack loss right
}

local cvColors = {
    color("1,1,0"), -- cv left hand left finger
    color("1,1,0"), -- cv left hand right finger
    color("1,0,0"), -- cv right hand left finger
    color("1,0,0"), -- cv right hand right finger
}

local techColors = {
    color("1,1,0"),		-- = pewp left hand
    color("0.7,1,0"),	-- = obliosis left hand
    color("0,1,1"),		-- = c left hand

    color("1,1,0"),		-- = pewp right hand
    color("0.7,1,0"),	-- = obliosis right hand
    color("0,1,1"),		-- = c right hand
}

-- these are all CalcDiffValue mods only
-- in the same order
local calcDiffValueColors = {
    color("#7d6b91"),   -- NPSBase
    color("#7d6b51"),   -- MSBase
    color("#8481db"),   -- JackBase
    color("#1d6b91"),   -- CJBase
    --color("#7d6b91"),
    --color("#8481db"),
    color("#cc4fa3"),   -- TechBase
    --color("#995fa3"),
    color("#f2b5fa"),   -- RMABase
    --color("#f2b5fa"),
    color("#6c969d"),   -- MSD
    --color("#6c969d"),
}

-- these mods are CalcDebugMisc mods only
local miscColors = {
    color("0,1,1"),     -- pts
    color("1,0,0"),     -- ptloss
    --color("1,0.4,0"),   -- jackptloss

    -- everything starting from here downwards are only mods in the orderedExtraExtraLowerMods table
    color("1,0,0"),     -- ptlossStream
    color("1,0,0"),     -- ptlossJumpstream
    color("1,0,0"),     -- ptlossHandstream
    color("1,0,0"),     -- ptlossChordjack
    color("1,0,0"),     -- ptlossTechnical
    color("#6c969d"),     -- msdStream
    color("#6c969d"),     -- msdJumpstream
    color("#6c969d"),     -- msdHandstream
    color("#6c969d"),     -- msdChordjack
    color("#6c969d"),     -- msdTechnical
}

-- a remapping of modnames to colors (any mod really please dont make 2 enums the same name)
local modToColor = {}
-- a remapping of modnames to shortnames (same note as above)
local modToShortname = {}
for i, mod in pairs(CalcPatternMod) do
    local mod = shortenEnum("CalcPatternMod", mod)
    modToColor[mod] = modColors[i]
    modToShortname[mod] = modnames[i]
end
for i, mod in pairs(CalcDiffValue) do
    local mod = shortenEnum("CalcDiffValue", mod)
    modToColor[mod] = calcDiffValueColors[i]
    -- set shortname if desired here
end
do -- scope hahaha
    local i = 1
    for _, mod in pairs(orderedExtraUpperMods) do
        modToColor[mod] = modColors[#CalcPatternMod + i]
        modToShortname[mod] = modnames[#CalcPatternMod + i]
        i = i + 1
    end
    for _, mod in pairs(orderedExtraExtraUpperMods) do
        modToColor[mod] = modColors[#CalcPatternMod + i]
        modToShortname[mod] = modnames[#CalcPatternMod + i]
        i = i + 1
    end
    i = 1
    for _, mod in pairs(orderedExtraLowerMods) do
        modToColor[mod] = miscColors[i]
        i = i + 1
    end
    for _, mod in pairs(orderedExtraExtraLowerMods) do
        modToColor[mod] = miscColors[i]
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

                local shortname = modToShortname[mod] .. (hand == 1 and "l" or "r")
                local modcolor = modToColor[mod]

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
        if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["SSRS"]) then
            self:settextf("Upper SSR: %.4f", math.max(unpack(ssrs[1])))
        else
            if diffGroups[activeDiffGroup]["Jack"] and steps then
                local jackpbm = 1.0013144
                local tappoints = steps:GetRelevantRadars()[1] * 2
                local maxpoints = tappoints * jackpbm
                local afterloss = maxpoints - jackLossSumRight - jackLossSumLeft
                local reqpoints = tappoints * 0.93
                self:settextf("Upper Bound: %.2f  |  Loss Sum L: %5.2f  |  Loss Sum R: %5.2f  |  Pt AfterLoss/Req/Max: %5.2f/%5.2f/%5.2f", lowerGraphMaxJack*0.9, jackLossSumLeft, jackLossSumRight, afterloss, reqpoints, maxpoints)
            elseif diffGroups[activeDiffGroup]["CV"] and steps then
                self:settextf("Upper Bound: %.2f  |  Lower Bound: %.2f  |  Average CVs -  LL = %5.2f | LR = %5.2f | RL = %5.2f | RR = %5.2f", cvmax, cvmin, cva[1], cva[2], cva[3], cva[4])
            elseif (diffGroups[activeDiffGroup]["Tech1"] or diffGroups[activeDiffGroup]["Tech2"] or diffGroups[activeDiffGroup]["Tech3"]) and steps then
                self:settextf("Upper Bound: %.2f  |  Lower Bound: %.2f  |  Avg pewpL = %5.2f pewpR = %5.2f  |  Avg oblioL = %5.2f oblioR = %5.2f  | Avg cL = %5.2f cR = %5.2f", 
                    math.max(techminmaxavg["Left"][1][2], techminmaxavg["Left"][2][2], techminmaxavg["Right"][1][2], techminmaxavg["Right"][2][2]),
                    math.min(techminmaxavg["Left"][1][1], techminmaxavg["Left"][2][1], techminmaxavg["Left"][3][1], techminmaxavg["Right"][1][1], techminmaxavg["Right"][2][1], techminmaxavg["Right"][3][1]),
                    techminmaxavg["Left"][1][3], techminmaxavg["Right"][1][3],
                    techminmaxavg["Left"][2][3], techminmaxavg["Right"][2][3],
                    techminmaxavg["Left"][3][3], techminmaxavg["Right"][3][3]
            )
            else
                self:settextf("Upper Bound: %.4f  |  Grindscaler: %5.2f", lowerGraphMax, grindscaler)
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
                    for i = 1, #graphVecs["JS"][1] do
                        local x = fitX(i, #graphVecs["JS"][1])
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

                local values = graphVecs[mod][hand]
                if not values or not values[1] then return end
                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    local x = fitX(i + firstSecond / getCurRateValue() - 1, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY1(values[i])
                    y = y + plotHeight / 2
                    setOffsetVerts(verts, x, y, colorToUse) 
                end

                if #verts <= 1 then
                    verts = {}
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

local function topGraphLineJackStam(mod, colorToUse, hand)
    return Def.ActorMultiVertex {
        InitCommand = function(self)
            self:y(plotHeight+5)
        end,
        DoTheThingCommand = function(self)
            if song and enabled then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
                
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = jackdiffs[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], finalSecond / 2 / getCurRateValue()) -- song length based positioning
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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Jack"]) then
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
                
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup][mod]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local verts = {}
                local values = graphVecs[mod][hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    local x = fitX(i + firstSecond  / getCurRateValue() - 1, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i], lowerGraphMin, lowerGraphMax)

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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup][mod]) then
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
                
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = jackdiffs[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][2], lowerGraphMin, lowerGraphMaxJack)

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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Jack"]) then
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
                
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Jack"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = jackdiffs[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][4], lowerGraphMin, lowerGraphMax)

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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Jack"]) then
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
                
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["CV"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local col = col == 1 and "Left" or "Right"
                local verts = {}
                local values = cvvals[hand][col]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][2], cvmin - 0.1, cvmax + 0.25)

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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["CV"]) then
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
                
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Tech" .. techValIndex]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local hand = hand == 1 and "Left" or "Right"
                local verts = {}
                local values = techvals[hand]
                if not values or not values[1] then return end

                for i = 1, #values do
                    --local x = fitX(i, #values) -- vector length based positioning
                    -- if used, final/firstsecond must be halved
                    -- they need to be halved because the numbers we use here are not half second interval based, but row time instead
                    local x = fitX(values[i][1], finalSecond / 2 / getCurRateValue()) -- song length based positioning
                    local y = fitY2(values[i][1+techValIndex], techminmaxavg[hand][techValIndex][1], techminmaxavg[hand][techValIndex][2] * 1.20)

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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["Tech" .. techValIndex]) then
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
            if song and enabled and #ssrs > 0 then
                self:SetVertices({})
                self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}

                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["SSRS"]) then
                    self:visible(true)
                else
                    self:visible(false)
                end

                local verts = {}

                for i = 1, #ssrs[lineNum] do
                    local x = fitX(i, #ssrs[lineNum]) -- vector length based positioning
                    --local x = fitX(i + firstSecond  / getCurRateValue() - 1, finalSecond / getCurRateValue()) -- song length based positioning
                    local y = fitY2(ssrs[lineNum][i])

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
                if activeDiffGroup == -1 or (diffGroups[activeDiffGroup] and diffGroups[activeDiffGroup]["SSRS"]) then
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
            o[#o+1] = topGraphLine(modname, modToColor[modname], h)
        end
    end
    i = 1
    for mod, _ in pairs(miscToUpperMods) do
        for h = 1,2 do
            -- dont have to shorten enum here because i did something dumb
            o[#o+1] = topGraphLine(mod, modToColor[mod], h)
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
            o[#o+1] = bottomGraphLineMSD(modname, modToColor[modname], h)
        end
    end
    i = 1
    for mod, _ in pairs(miscToLowerMods) do
        for h = 1,2 do
            o[#o+1] = bottomGraphLineMSD(mod, modToColor[mod], h)
        end
        i = i + 1
    end
end

-- SSR skillset lines
for i = 1,8 do
    o[#o+1] = bottomGraphLineSSR(i, skillsetColors[i])
end

-- Jack diff line(s)
for h = 1,2 do
    local colr = jackdiffColors[h]
    o[#o+1] = bottomGraphLineJack(colr, h)
end

-- jack stam
for h = 1,2 do
    o[#o+1] = topGraphLineJackStam("jack_stam", color("1,1,1,1"), h)
end

-- jack loss
for h = 1,2 do
    local colr = jackdiffColors[h+2]
    o[#o+1] = bottomGraphLineJackloss(colr, h)
end

-- cv vals
do
    local i = 1
    for h = 1,2 do
        for c = 1,2 do
            local colr = cvColors[i]
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
            local colr = techColors[i]
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