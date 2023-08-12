-- this is a "secret" screen displayed by right clicking the song banner
-- you can leave the screen by pressing escape or clicking one of the buttons in the top of the screen

local lastHovered = nil
local lastusedsong = nil
local focused = false
local updateCoolStuff = nil
local tt = Def.ActorFrame {
    Name = "CalcDebugFile",
    WheelSettledMessageCommand = function(self, params)
        lastHovered = params.hovered
        lastusedsong = params.song

        if focused then
            SCUFF.preview.resetmusic = false
            if lastusedsong ~= nil and SCUFF.preview.active then
                local top = SCREENMAN:GetTopScreen()
                if top.PlayCurrentSongSampleMusic then
                    -- reset music, force start, force full length
                    SCUFF.preview.resetmusic = true
                    SOUND:StopMusic()
                    top:PlayCurrentSongSampleMusic(true, true)
                end
            end
        end

        -- cascade visual update to everything
        self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
    end,
    ChangedStepsMessageCommand = function(self, params)
        lastusedsong = GAMESTATE:GetCurrentSong()
        updateCoolStuff()
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = params.steps})
    end,
    OpenCalcDebugMessageCommand = function(self)
        if focused then return end
        focused = true
        updateCoolStuff()
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
        self:playcommand("SlideOn")
    end,
    CloseCalcDebugMessageCommand = function(self)
        if not focused then return end
        focused = false
        self:playcommand("SlideOff")
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if not focused then return end
        focused = false
        self:playcommand("SlideOff", params)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if not focused then return end
        focused = false
        self:playcommand("SlideOff", params)
    end,
}

local ratios = {
    TopGap = 109 / 1080, -- height of the upper lip of the screen
    PreviewGraphHeight = 37 / 555,
    BPMTextLeftGap = 1400 / 1920,
    BPMNumberLeftGap = 1450 / 1920,
    BPMWidth = 300 / 1920,
    LengthTextLeftGap = 1200 / 1920,
    LengthNumberLeftGap = 1280 / 1920,
    LeftTextLeftGap = 1000 / 1920,
    RateTextLeftGap = 1527 / 1920,
}

local actuals = {
    TopGap = ratios.TopGap * SCREEN_HEIGHT,
    PreviewGraphHeight = ratios.PreviewGraphHeight * SCREEN_HEIGHT,
    BPMTextLeftGap = ratios.BPMTextLeftGap * SCREEN_WIDTH,
    BPMNumberLeftGap = ratios.BPMNumberLeftGap * SCREEN_WIDTH,
    BPMWidth = ratios.BPMWidth * SCREEN_WIDTH,
    LengthTextLeftGap = ratios.LengthTextLeftGap * SCREEN_WIDTH,
    LeftTextLeftGap = ratios.LeftTextLeftGap * SCREEN_WIDTH,
    LengthNumberLeftGap = ratios.LengthNumberLeftGap * SCREEN_WIDTH,
    RateTextLeftGap = ratios.RateTextLeftGap * SCREEN_WIDTH,
}

local showPosition = actuals.TopGap
local hidePosition = SCREEN_HEIGHT
local beginPosition = hidePosition
local animationSeconds = 0.1

------ text size
local titleTextSize = 0.85
local authorTextSize = 0.75
local creditTextSize = 0.65
local packTextSize = 0.65
local bpmTextSize = 0.75
local lengthTextSize = 0.75
local rateTextSize = 0.75
local textGap = 5
local edgeGap = 10
local textzoomFudge = 5
local buttonHoverAlpha = 0.6

------ other sizing
local notefieldZoom = 0.5
local previewGraphWidth = 64 * 4 * notefieldZoom * 1.2
local previewGraphHeight = actuals.PreviewGraphHeight * notefieldZoom * 1.5
local previewX = SCREEN_WIDTH - previewGraphWidth/2 - edgeGap
local previewY = previewGraphHeight / 2 + edgeGap

local msdBoxX = edgeGap
local msdBoxWidth = SCREEN_WIDTH * 0.075
local msdBoxSize = 25
local msdTextSize = 0.5

local upperLineGraphX = msdBoxX + msdBoxWidth + edgeGap
local upperLineGraphY = SCREEN_HEIGHT * 0.3
local lowerLineGraphX = msdBoxX + msdBoxWidth + edgeGap
local lowerLineGraphY = SCREEN_HEIGHT * 0.7
local lineGraphWidth = SCREEN_WIDTH * 0.75
local lineGraphHeight = 250


-- for SSR graph generator, modify these constants
local ssrLowerBoundWife = 0.90 -- left end of the graph
local ssrUpperBoundWife = 0.97 -- right end of the graph
local ssrResolution = 100 -- higher number = higher resolution graph (and lag)

-- state vars
local lowestssr = 0
local highestssr = 1
local lowerGraphMax = 0
local lowerGraphMaxJack = 0
local jackLossSumLeft = 0
local jackLossSumRight = 0
local upperGraphMax = 0
local upperGraphMin = 0
local firstSecond = 0
local finalSecond = 0
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
local debugstrings = ""
local techvals = {}
local techminmaxavg = {}

-- a scaling function which outputs a percentage based on a given scale
local function scale(x, lower, upper, scaledMin, scaledMax)
    local perc = (x - lower) / (upper - lower)
    return perc * (scaledMax - scaledMin) + scaledMin
end

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
	return x / lastX * lineGraphWidth - lineGraphWidth / 2
end

-- restrict the x bounds of the graph to a specific range of percentages
local function getGraphBounds(vec)
    -- if these vectors are empty, dont restrict
    if vec == nil or #vec[1] == 0 then
        return 0, 1
    else
        -- get the x position of the first and last item, normalize the position, and then convert them to percentages [0,1]
        return (fitX(firstSecond  / getCurRateValue(), finalSecond / getCurRateValue()) + lineGraphWidth/2) / lineGraphWidth, (fitX(#vec[1] + firstSecond, finalSecond / getCurRateValue()) + lineGraphWidth/2) / lineGraphWidth
    end
end

-- restrict the x bounds of the graph to a specific range of percentages
local function getGraphBoundsJack(vec)
    -- if these vectors are empty, dont restrict
    if vec == nil or #vec == 0 then
        return 0, 1
    else
        -- get the x position of the first and last item, normalize the position, and then convert them to percentages [0,1]
        return (fitX(vec[1][1], finalSecond/2 / getCurRateValue()) + lineGraphWidth/2) / lineGraphWidth, (fitX(vec[#vec][1], finalSecond/2 / getCurRateValue()) + lineGraphWidth/2) / lineGraphWidth
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

    highestssr = output[1][1]
    lowestssr = output[1][1]
    for ss,vals in ipairs(output) do
        for ind,val in ipairs(vals) do
            if val > highestssr then highestssr = val end
            if val < lowestssr then lowestssr = val end
        end
    end
    lowestssr = lowestssr - 1
    highestssr = highestssr + 1
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
updateCoolStuff = function()
    local song = GAMESTATE:GetCurrentSong()
    local steps = GAMESTATE:GetCurrentSteps()
    if song then
        -- account for rate separately
        -- double the output because intervals are half seconds
        firstSecond = steps:GetFirstSecond() * 2
        finalSecond = steps:GetLastSecond() * 2
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
    MESSAGEMAN:Broadcast("UpdateUpperGraph", {mods = mods})
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
        local steps = GAMESTATE:GetCurrentSteps()
        if #ssrs == 0 and steps then
            ssrs = getGraphForSteps(steps)
            MESSAGEMAN:Broadcast("UpdateSSRLines")
        end
    end

    MESSAGEMAN:Broadcast("UpdateLowerGraph")
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
    color("1,1,0"),		-- = pewp right hand

    color("0.7,1,0"),	-- = obliosis left hand
    color("0.7,1,0"),	-- = obliosis right hand

    color("0,1,1"),		-- = c left hand
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


local topgraph = nil
local bottomgraph = nil
local t = Def.ActorFrame {
    Name = "Frame",
    InitCommand = function(self)
        self:y(beginPosition)
        self:diffusealpha(0)
    end,
    BeginCommand = function(self)
        local snm = SCREENMAN:GetTopScreen():GetName()
        local anm = self:GetName()
        -- this keeps track of whether or not the user is allowed to use the keyboard to change tabs
        CONTEXTMAN:RegisterToContextSet(snm, "CalcDebug", anm)

        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            -- if locked out, dont allow
            if not CONTEXTMAN:CheckContextSet(snm, "CalcDebug") then return end
            if event.type == "InputEventType_FirstPress" then
                if event.DeviceInput.button == "DeviceButton_space" then
                    -- this should propagate off to the right places
                    self:GetParent():playcommand("CloseCalcDebug")
                end

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
        end)
    end,
    SlideOnCommand = function(self)
        self:finishtweening()
        self:diffusealpha(1)
        self:decelerate(animationSeconds)
        self:y(showPosition)
        MESSAGEMAN:Broadcast("HideWheel")
        MESSAGEMAN:Broadcast("HideRightFrame")
        local snm = SCREENMAN:GetTopScreen():GetName()
        CONTEXTMAN:ToggleContextSet(snm, "CalcDebug", true)
        if not SCUFF.preview.active then
            -- chart preview was not on
            SCUFF.preview.active = true
            if not SCUFF.preview.resetmusic and lastusedsong ~= nil then
                local top = SCREENMAN:GetTopScreen()
                if top.PlayCurrentSongSampleMusic then
                    -- reset music, force start, force full length
                    SCUFF.preview.resetmusic = true
                    SOUND:StopMusic()
                    top:PlayCurrentSongSampleMusic(true, true)
                end
            end
        end
    end,
    SlideOffCommand = function(self, params)
        self:finishtweening()
        self:decelerate(animationSeconds)
        self:y(hidePosition)
        self:diffusealpha(0)
        SCUFF.preview.active = false
        MESSAGEMAN:Broadcast("ShowWheel")
        if params == nil or params.tab == nil then
            MESSAGEMAN:Broadcast("GeneralTabSet", {tab = SCUFF.generaltabindex})
        end
        local snm = SCREENMAN:GetTopScreen():GetName()
        CONTEXTMAN:ToggleContextSet(snm, "CalcDebug", false)
    end,
}

t[#t+1] = Def.Quad {
    Name = "BG",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT - actuals.TopGap)
        self:diffusealpha(0.45)
        registerActorToColorConfigElement(self, "main", "PrimaryBackground")
    end,
}

t[#t+1] = Def.ActorFrame {
    Name = "SongInfoFrame",
    InitCommand = function(self)
        self:xy(edgeGap, edgeGap)
    end,
    BeginCommand = function(self)
        local title = self:GetChild("Title")
        local artist = self:GetChild("Artist")
        local credit = self:GetChild("Credit")
        local pack = self:GetChild("Pack")
        artist:y(title:GetY() + title:GetZoomedHeight() + textGap)
        credit:y(artist:GetY() + artist:GetZoomedHeight() + textGap)
        pack:y(credit:GetY() + credit:GetZoomedHeight() + textGap)
        self:GetParent():GetChild("MSDFrame"):y(pack:GetY() + pack:GetZoomedHeight() + textGap * 3)

        local bpm1 = self:GetChild("BPMText")
        local bpm2 = self:GetChild("BPMDisplay")
        local length1 = self:GetChild("LengthText")
        local length2 = self:GetChild("LengthNumbers")
        local rate = self:GetChild("Rate")
        local ypos = pack:GetY()
        bpm1:y(ypos)
        bpm2:y(ypos)
        length1:y(ypos)
        length2:y(ypos)
        rate:y(ypos)
    end,
    DisplayLanguageChangedMessageCommand = function(self)
        if not focused then return end
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
    end,

    LoadFont("Common Normal") .. {
        Name = "Title",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(titleTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / titleTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("%s", params.song:GetDisplayMainTitle())
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Artist",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(authorTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / authorTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("~ %s", params.song:GetDisplayArtist())
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Credit",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(creditTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / creditTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("Charter: %s", params.song:GetOrTryAtLeastToGetSimfileAuthor())
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Pack",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoom(packTextSize)
            self:settext(" ")
            self:maxwidth((SCREEN_WIDTH/2) / packTextSize)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if not focused then return end
            if params.song == nil then return end
            self:settextf("Pack: %s", params.song:GetGroupName())
        end,
    },
    LoadActorWithParams("stepsdisplay", {ratios = {
        Width = 0.5,
        DiffFrameLeftGap = 0,
        DiffFrameRightGap = 0,
        LeftTextLeftGap = 0,
    }, actuals = {
        Width = SCREEN_WIDTH,
        DiffFrameLeftGap = 0 * SCREEN_WIDTH,
        DiffFrameRightGap = previewGraphWidth + 64,
        LeftTextLeftGap = 0 * SCREEN_WIDTH,
        DiffFrameUpperGap = edgeGap/2,
    }}) .. {
        -- hmm
    },
    LoadFont("Common Normal") .. {
        Name = "BPMText",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:x(actuals.BPMTextLeftGap)
            self:zoom(bpmTextSize)
            self:maxwidth(math.abs(actuals.BPMNumberLeftGap - actuals.BPMTextLeftGap) / bpmTextSize - textzoomFudge)
            self:settext("BPM")
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end
    },
    Def.BPMDisplay {
        File = THEME:GetPathF("Common", "Normal"),
        Name = "BPMDisplay",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:x(actuals.BPMNumberLeftGap)
            self:zoom(bpmTextSize)
            self:maxwidth(actuals.BPMWidth / bpmTextSize - textzoomFudge)
        end,
        SetCommand = function(self, params)
            if params.steps then
                self:visible(true)
                self:SetFromSteps(params.steps)
            else
                self:visible(false)
            end
        end
    },
    LoadFont("Common Normal") .. {
        Name = "LengthText",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:x(actuals.LengthTextLeftGap)
            self:zoom(lengthTextSize)
            self:maxwidth(math.abs(actuals.LengthNumberLeftGap - actuals.LeftTextLeftGap) / lengthTextSize - textzoomFudge)
            self:settext("Length")
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "LengthNumbers",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:x(actuals.LengthNumberLeftGap)
            self:zoom(lengthTextSize)
            self:maxwidth(math.abs(actuals.BPMTextLeftGap - actuals.LengthNumberLeftGap) / lengthTextSize - textzoomFudge)
            self:settext("55:55")
        end,
        SetCommand = function(self, params)
            if params.steps then
                local len = GetPlayableTime()
                self:settext(SecondsToMMSS(len))
                self:diffuse(colorByMusicLength(len))
            else
                self:settext("--:--")
                self:diffuse(color("1,1,1,1"))
            end
        end
    },
    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "Rate",
        InitCommand = function(self)
            self:x(actuals.RateTextLeftGap)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")

            txt:halign(0):valign(0)
            txt:zoom(rateTextSize)
            txt:maxwidth(math.abs(SCREEN_WIDTH - actuals.RateTextLeftGap) / rateTextSize - textzoomFudge)
            registerActorToColorConfigElement(txt, "main", "PrimaryText")
            bg:halign(0):valign(0)
            bg:diffusealpha(0.7)
            txt:settext(" ")
            bg:zoomy(txt:GetZoomedHeight() * 1.2)
            bg:y(-txt:GetZoomedHeight() * 0.1)
        end,
        SetCommand = function(self, params)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")
            local str = string.format("%.2f", getCurRateValue()) .. "x"
            txt:settext(str)
            bg:zoomx(txt:GetZoomedWidth())
        end,
        ClickCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "OnMouseDown" then
                if params.event == "DeviceButton_left mouse button" then
                    changeMusicRate(1, true)
                elseif params.event == "DeviceButton_right mouse button" then
                    changeMusicRate(-1, true)
                end
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == 'in' then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end,
        MouseScrollMessageCommand = function(self, params)
            if self:IsInvisible() then return end
            if isOver(self:GetChild("BG")) then
                if params.direction == "Up" then
                    changeMusicRate(1, true)
                elseif params.direction == "Down" then
                    changeMusicRate(-1, true)
                end
            end
        end
    },
}

local function msditem(i)
    local skillset = ms.SkillSets[i]
    return Def.ActorFrame {
        Name = "msd_"..skillset,
        InitCommand = function(self)
            if i <= 1 then return end
            local c = self:GetParent():GetChild("msd_"..ms.SkillSets[i-1])
            self:y(c:GetY() + c:GetChild("skillset"):GetZoomedHeight() + textGap)
        end,

        LoadFont("Common Normal") .. {
            Name = "skillset",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:zoom(msdTextSize)
                self:x(msdBoxWidth/2)
                self:maxwidth((msdBoxWidth/2) / msdTextSize)
                self:settextf("%s:", skillset)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
        },
        LoadFont("Common Normal") .. {
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoom(msdTextSize)
                self:x(msdBoxWidth/2 + edgeGap/2)
                self:maxwidth(msdBoxWidth / msdTextSize)
            end,
            SetCommand = function(self, params)
                if focused then
                    if params.steps == nil then return end
                    local meter = params.steps:GetMSD(getCurRateValue(), i)
                    self:diffuse(colorByMSD(meter))
                    self:settextf("%5.2f", meter)
                end
            end,
        }
    }
end
local function msditems()
    local t = Def.ActorFrame {
        Name = "MSDContainer",
    }
    for i=1, #ms.SkillSets do
        t[#t+1] = msditem(i)
    end
    return t
end
t[#t+1] = Def.ActorFrame {
    Name = "MSDFrame",
    InitCommand = function(self)
        self:x(edgeGap)
    end,
    msditems(),
}

t[#t+1] = Def.ActorFrame {
    Name = "MiniChartPreviewFrame",
    InitCommand = function(self)
        self:xy(previewX, previewY)
    end,
    SetCommand = function(self, params)
        if not focused then return end
        self:playcommand("LoadNoteData", {steps = params.steps})
    end,

    Def.NoteFieldPreview {
        Name = "NoteField",
        DrawDistanceBeforeTargetsPixels = (SCREEN_HEIGHT - actuals.TopGap) / notefieldZoom,
        DrawDistanceAfterTargetsPixels = 0, -- notes disappear at the receptor
    
        InitCommand = function(self)
            self:xy(0, previewGraphHeight)
            self:zoom(notefieldZoom)
            -- make mods work
            self:SetFollowPlayerOptions(true)
            self:SetUpdateFunction(function(self)
                ArrowEffects.Update()
            end)
            self:show_interval_bars(true)
        end,
        BeginCommand = function(self)
            -- we need to redo the draw order for the notefield and graph
            -- the notefield ends up being on top of everything in the actorframe otherwise
            self:draworder(1)
            self:GetParent():GetChild("ChordDensityGraphFile"):draworder(2)
            self:GetParent():SortByDrawOrder()
        end,
        LoadNoteDataCommand = function(self, params)
            local steps = params.steps
            if steps ~= nil then
                self:LoadNoteData(steps, true)
            else
                self:LoadDummyNoteData()
            end
            self:SetConstantMini(ReceptorSizeToMini(notefieldZoom))
            self:UpdateYReversePixels(0)
        end,
        OptionUpdatedMessageCommand = function(self, params)
            if params ~= nil then
                -- listen for the notedata modifying mods being toggled and apply their changes immediately
                local options = {
                    Mirror = true,
                    Turn = true,
                    ["Pattern Transform"] = true,
                    ["Hold Transform"] = true,
                    Remove = true,
                    Insert = true,
                    Mines = true,
                    ["Scroll Direction"] = true,
                }
                if options[params.name] ~= nil then
                    self:playcommand("LoadNoteData", {steps = GAMESTATE:GetCurrentSteps()})
                end
    
                if params.name == "Music Wheel Position" then
                    self:playcommand("SetPosition")
                end
            end
        end,
    },
    LoadActorWithParams("../chordDensityGraph.lua", {sizing = {
        Width = previewGraphWidth,
        Height = previewGraphHeight,
        NPSThickness = 2,
        TextSize = 0,
    }}) .. {
        InitCommand = function(self)
            self:xy(-previewGraphWidth/2, -previewGraphHeight/2)
        end,
        LoadNoteDataCommand = function(self, params)
            local steps = params.steps
            self:playcommand("LoadDensityGraph", {steps = steps, song = params.song})
        end,
    }
}

t[#t+1] = Def.ActorFrame {
    Name = "UpperGraphHolder",
    InitCommand = function(self)
        self:xy(upperLineGraphX, upperLineGraphY)
        self.graphs = {}
        self:SetUpdateFunction(function(self)
            if not focused then return end
            local txt = self:GetChild("DebugStringText")
            local bar = self:GetChild("GraphSeekBar")
            local txt2 = self:GetChild("GraphText")
            local bg = self:GetChild("GraphTextBG")
            local graphbg = self:GetChild("BG")
            if isOver(graphbg) then
                local mx = INPUTFILTER:GetMouseX()
                local ypos = INPUTFILTER:GetMouseY() - self:GetTrueY()
                
                local w = graphbg:GetZoomedWidth() * graphbg:GetParent():GetTrueZoom()
                local leftEnd = graphbg:GetTrueX() - (graphbg:GetHAlign() * w)
                local rightEnd = graphbg:GetTrueX() + w - (graphbg:GetHAlign() * w)
                local perc = (mx - leftEnd) / (rightEnd - leftEnd)
                local goodXPos = perc * w
                
                txt:visible(true)
                txt:x(goodXPos + 36)
                txt:y(ypos - 20)
    
                local index = convertPercentToIndexForMods(mx - leftEnd, rightEnd - leftEnd)
                txt:settext(debugstrings[index])

                bar:visible(true)
                txt2:visible(true)
                bg:visible(true)
                bar:x(goodXPos)
                txt2:x(goodXPos - 4)
                txt2:y(ypos)
                bg:zoomto(txt2:GetZoomedWidth() + 6, txt2:GetZoomedHeight() + 6)
                bg:x(goodXPos)
                bg:y(ypos)
    
                local modText = getDebugModsForIndex(CalcPatternMod, "CalcPatternMod", miscToUpperMods, index, true)
                txt2:settext(modText)
            else
                txt:visible(false)
                txt2:visible(false)
                bar:visible(false)
                bg:visible(false)
            end
        end)
    end,
    UpdateUpperGraphMessageCommand = function(self)
        if activeModGroup == -1 then return end
        if debugGroups[activeModGroup] then
            self:playcommand("DrawNothing")
            for n,_ in pairs(debugGroups[activeModGroup]) do
                local dataL = {}
                local dataR = {}
                local ymin = 0
                local ymax = 0
                for i=1, #graphVecs[n][1] do
                    local l = graphVecs[n][1][i]
                    local r = graphVecs[n][2][i]
                    dataL[#dataL+1] = {i, l}
                    dataR[#dataR+1] = {i, r}
                    if l > ymax then ymax = l end
                    if l < ymin then ymin = l end
                    if r > ymax then ymax = r end
                    if r < ymin then ymin = r end
                end
                -- just force these for now...
                ymin = 0
                ymax = 1.5
                self:playcommand("ShowGraph", {
                    name = n .. "Left",
                    data = dataL,
                    xmin = 1,
                    xmax = #dataL,
                    ymin = ymin,
                    ymax = ymax,
                    color = modToColor[n] or color("#FFFFFF"),
                })
                self:playcommand("ShowGraph", {
                    name = n .. "Right",
                    data = dataR,
                    xmin = 1,
                    xmax = #dataR,
                    ymin = ymin,
                    ymax = ymax,
                    color = modToColor[n] or color("#FFFFFF"),
                })
            end
        end
    end,
    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            topgraph = self
            self:halign(0)
            self:zoomto(lineGraphWidth, lineGraphHeight)
            self:diffusealpha(0.7)
            registerActorToColorConfigElement(self, "main", "PrimaryBackground")
        end,
    },
    Def.Quad {
        Name = "GraphTextBG",
        InitCommand = function(self)
            self:y(8 + lineGraphHeight+5):halign(1):draworder(1100):diffuse(color("0,0,0,.4")):zoomto(20,20)
        end
    },
    LoadFont("Common Normal") .. {
        Name = "GraphText",
        InitCommand = function(self)
            self:y(8 + lineGraphHeight+5):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.4)
        end
    },
    LoadFont("Common Normal") .. {
        Name = "DebugStringText",
        InitCommand = function(self)
            self:y(8 + lineGraphHeight+5):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.5):maxheight(500)
        end
    },
    Def.Quad {
        Name = "GraphSeekBar",
        InitCommand = function(self)
            self:zoomto(1, lineGraphHeight):diffuse(color("1,.2,.5,1")):draworder(1100)
        end
    },
    ShowGraphCommand = function(self, params)
        local graphname = params.name
        if self.graphs[graphname] == nil then
            self:AddChildFromPath(THEME:GetPathG("", "lineGraph"))
            self:playcommand("RegisterToGraphOwner", {name = params.name})
        end
        self.graphs[graphname]:playcommand("DisplayGraph", {
            data = params.data,
            xmin = params.xmin,
            xmax = params.xmax,
            ymin = params.ymin,
            ymax = params.ymax,
            color = params.color,
        })
    end
}

t[#t+1] = Def.ActorFrame {
    Name = "LowerGraphHolder",
    InitCommand = function(self)
        self:xy(lowerLineGraphX, lowerLineGraphY)
        self.graphs = {}
        self:SetUpdateFunction(function(self)
            if not focused then return end
            local bar = self:GetChild("Seek2")
            local txt = self:GetChild("Seektext2")
            local bg = self:GetChild("Seektext2BG")
            local graphbg = self:GetChild("BG")
            if isOver(graphbg) then
                local mx = INPUTFILTER:GetMouseX()
                local ypos = INPUTFILTER:GetMouseY() - self:GetTrueY()
                
                local w = graphbg:GetZoomedWidth() * graphbg:GetParent():GetTrueZoom()
                local leftEnd = graphbg:GetTrueX() - (graphbg:GetHAlign() * w)
                local rightEnd = graphbg:GetTrueX() + w - (graphbg:GetHAlign() * w)
                local perc = (mx - leftEnd) / (rightEnd - leftEnd)
                local goodXPos = perc * w

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
        end)
    end,
    UpdateLowerGraphMessageCommand = function(self)
        if activeDiffGroup == -1 then return end
        if diffGroups[activeDiffGroup] then
            self:playcommand("DrawNothing")
            for n,_ in pairs(diffGroups[activeDiffGroup]) do
                local dataL = {}
                local dataR = {}
                local ymin = 0
                local ymax = 0
                local srcdata = {}
                if _ and n == "SSRS" then
                    -- ssrs
                    local data = {}
                    for i=1, #ssrs do
                        local data = {}
                        local ymin = 0
                        local ymax = 0
                        for ind, v in ipairs(ssrs[i]) do
                            data[#data+1] = {ind, v}
                            if v > ymax then ymax = v end
                        end
                        self:playcommand("ShowGraph", {
                            name = n .. i,
                            data = data,
                            xmin = 1,
                            xmax = #data,
                            ymin = lowestssr,
                            ymax = highestssr,
                            color = skillsetColors[i] or color("#FFFFFF"),
                        })
                    end
                    return
                end
                if _ and n == "Jack" then
                    -- jack diffs, jack stam, and jack loss
                    for h=1,2 do
                        local diffdata = {}
                        local lossdata = {}
                        local stamdata = {}
                        local ht = h==1 and "Left" or "Right"
                        for i=1, #jackdiffs[ht] do
                            local time = jackdiffs[ht][i][1]
                            local diff = jackdiffs[ht][i][2]
                            local stam = jackdiffs[ht][i][3]
                            local loss = jackdiffs[ht][i][4]
                            diffdata[#diffdata+1] = {time, diff}
                            lossdata[#lossdata+1] = {time, loss}
                            stamdata[#stamdata+1] = {time, stam}
                        end
                        self:playcommand("ShowGraph", {
                            name = "JackLoss"..ht,
                            data = lossdata,
                            xmin = 0,
                            xmax = finalSecond / 2 / getCurRateValue(),
                            ymin = 0,
                            ymax = lowerGraphMax,
                            color = jackdiffColors[h+2] or color("#FFFFFF"),
                        })
                        self:playcommand("ShowGraph", {
                            name = "JackDiff"..ht,
                            data = diffdata,
                            xmin = 0,
                            xmax = finalSecond / 2 / getCurRateValue(),
                            ymin = 0,
                            ymax = lowerGraphMaxJack,
                            color = jackdiffColors[h] or color("#FFFFFF"),
                        })
                        self:playcommand("ShowGraph", {
                            name = "JackStam"..ht,
                            data = stamdata,
                            xmin = 0,
                            xmax = finalSecond / 2 / getCurRateValue(),
                            ymin = 0,
                            ymax = 1.5, -- like the top graph
                            color = color("#FFFFFF"),
                        })
                    end
                    return
                end
                if _ and n == "CV" then
                    -- interval coeff. variance
                    local i = 1
                    for hand = 1,2 do
                        for col = 1,2 do
                            local color = cvColors[i]
                            i = i + 1
                            local handStr = hand == 1 and "Left" or "Right"
                            local colStr = col == 1 and "Left" or "Right"

                            self:playcommand("ShowGraph", {
                                name = "CV"..hand..col,
                                data = cvvals[handStr][colStr],
                                xmin = 0,
                                xmax = finalSecond / 2 / getCurRateValue(),
                                ymin = cvmin - 0.1,
                                ymax = cvmax + 0.25,
                                color = color or color("#FFFFFF"),
                            })
                        end
                    end
                    return
                end
                if _ and (n == "Tech1" or n == "Tech2" or n == "Tech3") then
                    -- tech 1=pewp, 2=obliosis, 3=c
                    local techI = tonumber(n:sub(-1))
                    for h=1,2 do
                        local hand = h==1 and "Left" or "Right"
                        local colorI = (techI-1) * 2 + h

                        local data = {}
                        for i=1,#techvals[hand] do
                            local time = techvals[hand][i][1]
                            local val = techvals[hand][i][1+techI]
                            data[#data+1] = {time, val}
                        end

                        self:playcommand("ShowGraph", {
                            name = n..hand,
                            data = data,
                            xmin = 0,
                            xmax = finalSecond / 2 / getCurRateValue(),
                            ymin = techminmaxavg[hand][techI][1],
                            ymax = techminmaxavg[hand][techI][2] * 1.2,
                            color = techColors[colorI] or color("#FFFFFF"),
                        })
                    end
                    return
                end

                -- the rest of these should be regular diffGroups entries

                local dataL = {}
                local dataR = {}
                for i=1, #graphVecs[n][1] do
                    local l = graphVecs[n][1][i]
                    local r = graphVecs[n][2][i]
                    dataL[#dataL+1] = {i + firstSecond / getCurRateValue() - 1, l}
                    dataR[#dataR+1] = {i + firstSecond / getCurRateValue() - 1, r}
                end

                self:playcommand("ShowGraph", {
                    name = n .. "Left",
                    data = dataL,
                    xmin = 0,
                    xmax = finalSecond / getCurRateValue(),
                    ymin = lowerGraphMin,
                    ymax = lowerGraphMax,
                    color = modToColor[n] or color("#FFFFFF"),
                })
                self:playcommand("ShowGraph", {
                    name = n .. "Right",
                    data = dataR,
                    xmin = 0,
                    xmax = finalSecond / getCurRateValue(),
                    ymin = lowerGraphMin,
                    ymax = lowerGraphMax,
                    color = modToColor[n] or color("#FFFFFF"),
                })
            end
        end
    end,
    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            bottomgraph = self
            self:halign(0)
            self:zoomto(lineGraphWidth, lineGraphHeight)
            self:diffusealpha(0.7)
            registerActorToColorConfigElement(self, "main", "PrimaryBackground")
        end,
    },
    Def.Quad {
        Name = "Seektext2BG",
        InitCommand = function(self)
            self:y(8 + lineGraphHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("0,0,0,.4")):zoomto(20,20)
        end
    },
    LoadFont("Common Normal") .. {
        Name = "Seektext2",
        InitCommand = function(self)
            self:y(8 + lineGraphHeight+5):valign(1):halign(1):draworder(1100):diffuse(color("1,1,1")):zoom(0.4)
        end
    },
    Def.Quad {
        Name = "Seek2",
        InitCommand = function(self)
            self:zoomto(1, lineGraphHeight):diffuse(color("1,.2,.5,1")):draworder(1100)
        end
    },
    ShowGraphCommand = function(self, params)
        local graphname = params.name
        if self.graphs[graphname] == nil then
            self:AddChildFromPath(THEME:GetPathG("", "lineGraph"))
            self:playcommand("RegisterToGraphOwner", {name = params.name})
        end
        self.graphs[graphname]:playcommand("DisplayGraph", {
            data = params.data,
            xmin = params.xmin,
            xmax = params.xmax,
            ymin = params.ymin,
            ymax = params.ymax,
            color = params.color,
        })
    end
}

t[#t+1] = Def.Actor {
    OnCommand = function(self)
        local data = {
            {0,0},
            {1,1},
            {2,2},
            {3,3},
            {3.5,10},
            {3.6,0},
            {4,4},
            {5,5},
            --{10,10},
        }
        self:GetParent():GetChild("LowerGraphHolder"):playcommand("ShowGraph", {
            name = "test", data = data, ymin = 0, ymax = 10
        })
        self:GetParent():GetChild("UpperGraphHolder"):playcommand("ShowGraph", {
            name = "test", data = data, ymin = 0, ymax = 10
        })
    end,
}


tt[#tt+1] = t
return tt