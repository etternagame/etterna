--- Calc debug utils so we dont copy paste it 15 times
-- like the 9th utility file
-- @module 99_calcdebugutil

CALC = {
    -- for SSR graph generator, modify these constants
    ssrLowerBoundWife = 0.90, -- left end of the graph
    ssrUpperBoundWife = 0.97, -- right end of the graph
    ssrResolution = 100, -- higher number = higher resolution graph (and lag)

    debugstate = {},

    -- this is a huge nested table of all the calc debug enums
    CalcDebugTypes = {
        CalcPatternMod = CalcPatternMod,
        CalcDiffValue = CalcDiffValue,
        CalcDebugMisc = CalcDebugMisc,
    },

    -- list of all additional enums to include in the upper graph
    -- it is assumed these are members of CalcDebugMisc
    miscToUpperMods = {
        StamMod = true,

        -- these mods are not really in the CalcDebugMisc enum. they are not real.
        -- if this gets messed up things do not work
        TotalPatternModStream = true,
        TotalPatternModJumpstream = true,
        TotalPatternModHandstream = true,
        TotalPatternModChordjack = true,
        TotalPatternModTechnical = true,
    },

    -- list of all additional enums to include in the lower graph
    -- it is assumed these are members of CalcDebugMisc (they arent)
    miscToLowerMods = {
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
    },

    -- this list is used for functional purposes to keep the order of the lists generated in a certain order
    -- particularly, it's the order determined by the enums on the c++ side
    -- you have to see it to believe it, but it really does work
    orderedExtraUpperMods = {},
    orderedExtraLowerMods = {},

    -- convolution
    upperExtraExtraMods = {
        TotalPatternModStream = true,
        TotalPatternModJumpstream = true,
        TotalPatternModHandstream = true,
        TotalPatternModChordjack = true,
        TotalPatternModTechnical = true,
    },
    orderedExtraExtraUpperMods = {
        "TotalPatternModStream",
        "TotalPatternModJumpstream",
        "TotalPatternModHandstream",
        "TotalPatternModChordjack",
        "TotalPatternModTechnical",
    },
    lowerExtraExtraMods = {
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
    },
    orderedExtraExtraLowerMods = {
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
    },

    -- specify enum names as tables here
    -- any number allowed
    -- there is no order to anything in the groups, only the groups themselves
    debugGroups = {
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
            OldAnchorScaler = true,
            OldRollScaler = true,
            OldOHJScaler = true,
            OldJumpScaler = true,
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
        {   -- Group 18
            GenericStream = true,
        },
        {   -- Group 19
            GenericChordstream = true,
        },
        {   -- Group 20
            GenericBracketing = true,
        },
        {   -- Group 21
            HandSwitch = true,
        }
    },

    -- specify enum names here
    -- only CalcDiffValue enums
    -- also specify SSR to show all SSRs (recommend to leave a group alone)
    -- also specify Jack to show the Jack diffs for both hands
    -- miscDebugMods that are also specified under miscToLowerMods can be placed here
    diffGroups = {
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
    },

    
    --[[ enum mapping for downscaler things:
        this list has order and should match the enums used
    ]]
    modnames = {
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
        "gstrea",
        "gchstr",
        "gbrack",
        "hsw",
        "_anch", -- old mod
        "_jump", -- old mod
        "_ohj", -- old mod
        "_roll", -- old mod
        "hbalnc",

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
    },

    -- this list has order
    -- try to keep it exactly in the order of the enums used :)
    modColors = {
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
        color("1,1,1"), -- genericstream
        color("1,1,1"), -- genericchordstream
        color("1,1,1"), -- genericbracketing
        color("1,1,1"), -- handswitchmod
        color(".1,.3,.9"), -- oldanchormod
        color("1,1,0"), -- oldjumpmod
        color("1,0.4,0"), -- oldohjmod
        color("0,1,0"), -- oldrollmod
        color("1,0.2,1"), -- handbal


        -- place CalcPatternMod Colors above this line
        -- MISC MODS START HERE (same order as miscToUpperMods)
        color("0.7,1,0"),		-- lime			= stam

        -- everything starting from here downwards are only mods in the orderedExtraExtraUpperMods table
        color("0.7,1,0"),		-- lime			= totalpatternmodstream
        color("0.7,1,0"),		-- lime			= totalpatternmodjs
        color("0.7,1,0"),		-- lime			= totalpatternmodhs
        color("0.7,1,0"),		-- lime			= totalpatternmodcj
        color("0.7,1,0"),		-- lime			= totalpatternmodtech
    },

    skillsetColors = {
        color("1,1,1"),     -- overall
        color("#333399"),   -- stream
        color("#6666ff"),   -- jumpstream
        color("#cc33ff"),   -- handstream
        color("#ff99cc"),   -- stamina
        color("#009933"),   -- jack
        color("#66ff66"),   -- chordjack
        color("#808080"),    -- tech
    },

    jackdiffColors = {
        color("1,1,0"), -- jack diff left
        color(".6,0,.7"), -- jack diff right
        color("1,0,0,1"), -- jack loss left
        color("1,0,0,1"), -- jack loss right
    },

    cvColors = {
        color("1,1,0"), -- cv left hand left finger
        color("1,1,0"), -- cv left hand right finger
        color("1,0,0"), -- cv right hand left finger
        color("1,0,0"), -- cv right hand right finger
    },

    techColors = {
        color("1,1,0"),		-- = pewp left hand
        color("0.7,1,0"),	-- = obliosis left hand
        color("0,1,1"),		-- = c left hand

        color("1,1,0"),		-- = pewp right hand
        color("0.7,1,0"),	-- = obliosis right hand
        color("0,1,1"),		-- = c right hand
    },

    -- these are all CalcDiffValue mods only
    -- in the same order
    calcDiffValueColors = {
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
    },

    -- these mods are CalcDebugMisc mods only
    miscColors = {
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
    },
    -- a remapping of modnames to colors (any mod really please dont make 2 enums the same name)
    modToColor = {},
    -- a remapping of modnames to shortnames (same note as above)
    modToShortname = {},
}


---- CALC static vars runtime init ----------
---------------------------------------------
for i, mod in pairs(CalcDebugMisc) do
    local mod = shortenEnum("CalcDebugMisc", mod)
    if CALC.miscToUpperMods[mod] then
        CALC.orderedExtraUpperMods[#CALC.orderedExtraUpperMods+1] = mod
    end
end
for i, mod in pairs(CalcDebugMisc) do
    local mod = shortenEnum("CalcDebugMisc", mod)
    if CALC.miscToLowerMods[mod] then
        CALC.orderedExtraLowerMods[#CALC.orderedExtraLowerMods+1] = mod
    end
end
for i, mod in pairs(CalcPatternMod) do
    local mod = shortenEnum("CalcPatternMod", mod)
    CALC.modToColor[mod] = CALC.modColors[i]
    CALC.modToShortname[mod] = CALC.modnames[i]
end
for i, mod in pairs(CalcDiffValue) do
    local mod = shortenEnum("CalcDiffValue", mod)
    CALC.modToColor[mod] = CALC.calcDiffValueColors[i]
    -- set shortname if desired here
end
do -- scope hahaha
    local i = 1
    for _, mod in pairs(CALC.orderedExtraUpperMods) do
        CALC.modToColor[mod] = CALC.modColors[#CalcPatternMod + i]
        CALC.modToShortname[mod] = CALC.modnames[#CalcPatternMod + i]
        i = i + 1
    end
    for _, mod in pairs(CALC.orderedExtraExtraUpperMods) do
        CALC.modToColor[mod] = CALC.modColors[#CalcPatternMod + i]
        CALC.modToShortname[mod] = CALC.modnames[#CalcPatternMod + i]
        i = i + 1
    end
    i = 1
    for _, mod in pairs(CALC.orderedExtraLowerMods) do
        CALC.modToColor[mod] = CALC.miscColors[i]
        i = i + 1
    end
    for _, mod in pairs(CALC.orderedExtraExtraLowerMods) do
        CALC.modToColor[mod] = CALC.miscColors[i]
        i = i + 1
    end
end
---------------------------------------------


function CALC.initdebugstate(self)
    self.debugstate = {
        -- used for positioning graphs by time instead of vector length
        finalSecond = 0,
        firstSecond = 0,
        steplength = 0,

        -- data and calculated data
        graphVecs = {},
        jackdiffs = {},
        cvvals = {},
        cva = {},
        cvmax = 1,
        cvmin = 0,
        ssrs = {},
        grindscaler = 0,
        activeModGroup = 1,
        activeDiffGroup = 1,
        debugstrings = {},
        techvals = {},
        techminmaxavg = {},

        highest = 0,
        lowest = 0,
        lowerGraphMax = 0,
        lowerGraphMaxJack = 0,
        jackLossSumLeft = 0,
        jackLossSumRight = 0,
        upperGraphMax = 0,
        lowerGraphMin = 0,
        upperGraphMin = 0,
    }
end
CALC:initdebugstate()


function CALC:calcCVA()
    self.debugstate.cva = {0,0,0,0}
    self.debugstate.cvmax = -1
    self.debugstate.cvmin = 999
    if #self.debugstate.cvvals["Left"]["Left"] == 0 and
        #self.debugstate.cvvals["Left"]["Right"] == 0 and
        #self.debugstate.cvvals["Right"]["Left"] == 0 and
        #self.debugstate.cvvals["Right"]["Right"] == 0 then
        return
    end
    local i = 1
    for h, hand in pairs(self.debugstate.cvvals) do
        for c, col in pairs(hand) do
            local sum = 0
            for _, v in ipairs(col) do
                if v[2] < self.debugstate.cvmin then self.debugstate.cvmin = v[2] end
                if v[2] > self.debugstate.cvmax then self.debugstate.cvmax = v[2] end
                sum = sum + v[2]
            end
            self.debugstate.cva[i] = sum / #col
            i = i + 1
        end
    end
end

function CALC:calcTVA()
    self.debugstate.techminmaxavg = {
        Left = {
            {999,0,0},{999,0,0},{999,0,0}
        },
        Right = {
            {999,0,0},{999,0,0},{999,0,0}
        }
    }
    if #self.debugstate.techvals["Left"] == 0 and
        #self.debugstate.techvals["Right"] == 0 then
        return
    end
    for h, hand in pairs(self.debugstate.techvals) do
        local sum = {0,0,0}
        for _, v in ipairs(hand) do
            if v[2] < self.debugstate.techminmaxavg[h][1][1] then self.debugstate.techminmaxavg[h][1][1] = v[2] end
            if v[2] > self.debugstate.techminmaxavg[h][1][2] then self.debugstate.techminmaxavg[h][1][2] = v[2] end
            if v[3] < self.debugstate.techminmaxavg[h][2][1] then self.debugstate.techminmaxavg[h][2][1] = v[3] end
            if v[3] > self.debugstate.techminmaxavg[h][2][2] then self.debugstate.techminmaxavg[h][2][2] = v[3] end
            if v[4] < self.debugstate.techminmaxavg[h][3][1] then self.debugstate.techminmaxavg[h][3][1] = v[4] end
            if v[4] > self.debugstate.techminmaxavg[h][3][2] then self.debugstate.techminmaxavg[h][3][2] = v[4] end
            sum[1] = sum[1] + v[2]
            sum[2] = sum[2] + v[3]
            sum[3] = sum[3] + v[4]
        end
        self.debugstate.techminmaxavg[h][1][3] = sum[1] / #hand
        self.debugstate.techminmaxavg[h][2][3] = sum[2] / #hand
        self.debugstate.techminmaxavg[h][3][3] = sum[3] / #hand
    end
end

-- graph percentage to index (independent of song length)
function CALC:convertPercentToIndexForSSRS(x)
    return getVectorIndexFromPercentage(x, self.debugstate.ssrs)
end

-- graph percentage to index
function CALC:convertPercentToIndexForGraphVec(x)
    return getVectorIndexFromPercentage(x, self.debugstate.graphVecs["JS"])
end

function CALC:produceThisManySSRs(steps, rate)
    local count = self.ssrResolution
    if count < 10 then count = 10 end
    local output = {}
    for j = 1,8 do output[j] = {0,0,0,0,0,0,0,0} end

    for i = 1, count do
        local values = steps:GetSSRs(rate, self.ssrLowerBoundWife + ((self.ssrUpperBoundWife - self.ssrLowerBoundWife) / count) * i)
        for j = 1,8 do
            output[j][i] = values[j]
        end
    end

    return output
end

function CALC:getGraphForSteps(steps)
    local output = self:produceThisManySSRs(steps, getCurRateValue())

    self.debugstate.highest = output[1][1]
    self.debugstate.lowest = output[1][1]
    for ss,vals in ipairs(output) do
        for ind,val in ipairs(vals) do
            if val > self.debugstate.highest then self.debugstate.highest = val end
            if val < self.debugstate.lowest then self.debugstate.lowest = val end
        end
    end
    self.debugstate.lowest = self.debugstate.lowest - 1
    self.debugstate.highest = self.debugstate.highest + 1
    return output
end

-- get a list of the mods that are active
-- indexes pointing to enum strings
function CALC:getActiveDebugMods()
    local output = {}

    if self.debugstate.activeModGroup > #self.debugGroups or self.debugstate.activeModGroup < 1 then return output end

    -- once for each hand, add it to the list
    for mod,_ in pairs(self.debugGroups[self.debugstate.activeModGroup]) do
        output[#output+1] = mod
        output[#output+1] = mod
    end

    return output
end

function CALC:loadSongLengthInfo(song, steps)
    if song and steps then
        -- account for rate separately
        -- double the output because intervals are half seconds
        self.debugstate.firstSecond = steps:GetFirstSecond() * 2
        self.debugstate.finalSecond = steps:GetLastSecond() * 2
        self.debugstate.steplength = (self.debugstate.finalSecond - self.debugstate.firstSecond) -- this is "doubled" here
    end
end

function CALC:preinitDebugValPhase()
    self.debugstate.jackdiffs = {Left = {}, Right = {}}
    self.debugstate.cvvals = {Left = {Left = {}, Right = {}}, Right = {Left = {}, Right = {}}}
    self.debugstate.techvals = {Left = {}, Right = {}}
end

function CALC:loadDebugDataForSteps(steps)
    if steps == nil then
        self.debugstate.graphVecs = {}
        return
    end

    -- Only load SSRs if currently displaying them; this is a major slowdown
    if self.diffGroups[self.debugstate.activeDiffGroup]["SSRS"] then
        self.debugstate.ssrs = self:getGraphForSteps(steps)
    else
        self.debugstate.ssrs = {}
    end
    self.debugstate.lowerGraphMax = 0
    self.debugstate.lowerGraphMaxJack = 0
    self.debugstate.jackLossSumLeft = 0
    self.debugstate.jackLossSumRight = 0
    local bap = steps:GetCalcDebugOutput() -- this must be called first before GetCalcDebugExt and GetCalcDebugJack
    local pap = steps:GetCalcDebugExt()
    self.debugstate.debugstrings = steps:GetDebugStrings()

    self.debugstate.grindscaler = bap["Grindscaler"]

    -- Jack debug output got hyper convoluted so im trying to make it as sane as possible
    -- basically jackdiffs[hand][index] = {row time, diff, stam, loss}
    -- this is so we can place the indices based on row time instead of index
    -- also keep in mind the row times are already changed for each rate so 1.1 will be smaller than 1.0
    -- also all the row times are relative to the first non empty noterow so lets just pad it by the firstsecond/2 too
    -- the odd logic below is done because the length of the hand vectors are often different, but the stam and diff vectors are the same
    -- that allows us to combine the two
    -- the reassignment is done based on the longest vector so 2n iterations are not necessary
    -- (in hindsight this is probably exactly the same runtime whatever)
    local jackdbg = steps:GetCalcDebugJack()["JackHand"]
    if jackdbg then
        local upperiter = #jackdbg["Left"] > #jackdbg["Right"] and #jackdbg["Left"] or #jackdbg["Right"]
        for i = 1, upperiter do
            if jackdbg["Left"][i] then
                self.debugstate.jackdiffs["Left"][#self.debugstate.jackdiffs["Left"] + 1] = {
                    jackdbg["Left"][i][1] + self.debugstate.firstSecond/2/getCurRateValue(),
                    jackdbg["Left"][i][2],
                    jackdbg["Left"][i][3],
                    jackdbg["Left"][i][4]
                }
                if jackdbg["Left"][i][2] > self.debugstate.lowerGraphMaxJack then
                    self.debugstate.lowerGraphMaxJack = jackdbg["Left"][i][2]
                end
                self.debugstate.jackLossSumLeft = self.debugstate.jackLossSumLeft + jackdbg["Left"][i][4]
            end
            if jackdbg["Right"][i] then
                self.debugstate.jackdiffs["Right"][#self.debugstate.jackdiffs["Right"] + 1] = {
                    jackdbg["Right"][i][1] + self.debugstate.firstSecond/2/getCurRateValue(),
                    jackdbg["Right"][i][2],
                    jackdbg["Right"][i][3],
                    jackdbg["Right"][i][4]
                }
                if jackdbg["Right"][i][2] > self.debugstate.lowerGraphMaxJack then
                    self.debugstate.lowerGraphMaxJack = jackdbg["Right"][i][2]
                end
                self.debugstate.jackLossSumRight = self.debugstate.jackLossSumRight + jackdbg["Right"][i][4]
            end
        end
    end
    -- squeeze graph
    self.debugstate.lowerGraphMaxJack = self.debugstate.lowerGraphMaxJack / 0.9

    -- for each debug output type and its corresponding list of values
    for debugtype, sublist in pairs(self.CalcDebugTypes) do
        -- for each value in that list
        for i = 1, #sublist do

            -- translate the output list to our "cleaner" format
            local modname = shortenEnum(debugtype, sublist[i])
            self.debugstate.graphVecs[modname] = {}

            -- for each hand
            for h = 1, 2 do
                self.debugstate.graphVecs[modname][h] = bap[debugtype][modname][h]

                -- we set the bound of the lower graph to the max value of all debug output for it
                if debugtype == "CalcDiffValue" then
                    for j = 1, #self.debugstate.graphVecs[modname][h] do
                        local val = self.debugstate.graphVecs[modname][h][j]
                        if val > self.debugstate.lowerGraphMax then self.debugstate.lowerGraphMax = val end
                    end
                end
            end
        end
    end

    -- even more debug output
    local function fc(arr, name, fallbackValue, top)
        for i, ss in ipairs(ms.SkillSets) do
            self.debugstate.graphVecs[name..ss] = {}
            for h = 1,2 do
                local hand = h == 1 and "Left" or "Right"
                self.debugstate.graphVecs[name..ss][h] = {}
                if arr ~= nil then
                    for j = 1, #arr[hand][i] do
                        local val = arr[hand][i][j]
                        if val ~= val or val == nil or val == math.huge or val == -math.huge then val = fallbackValue end -- get rid of nan and nil
                        if top then
                            if val > self.debugstate.upperGraphMax then self.debugstate.upperGraphMax = val end
                        else
                            if val > self.debugstate.lowerGraphMax then self.debugstate.lowerGraphMax = val end
                        end
                        self.debugstate.graphVecs[name..ss][h][j] = val
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
                    self.debugstate.cvvals[hand][col][i] = {
                        vv[1] + self.debugstate.firstSecond/2/getCurRateValue(),
                        vv[2]
                    }
                end
            end
        end
        for hand, handvals in pairs(pap["DebugTechVals"]) do
            for i, vv in ipairs(handvals) do
                if (vv[2] > 10) then vv[2] = 0 end
                self.debugstate.techvals[hand][i] = {
                    vv[1] + self.debugstate.firstSecond/2/getCurRateValue(),
                    vv[2],
                    vv[3],
                    1 / vv[4]
                }
            end
        end
    end
    self:calcCVA()
    self:calcTVA()

    self.debugstate.upperGraphMin = 0.3
    self.debugstate.upperGraphMax = 1.25
end

function CALC:announceActiveDebugMods()
    local mods = self:getActiveDebugMods()
    MESSAGEMAN:Broadcast("UpdateActiveMods", {mods = mods})
    MESSAGEMAN:Broadcast("UpdateAverages", {mods = mods})
end

--[[
    Active modgroups are groups of Calc Debug Mods that selectively show
    We shouldn't ever show all of them unless otherwise noted because that's a lot of info and it's laggy
    (in any event, though, an invalid activeModGroup value will result in all modgroups appearing)
    ((-1 is meant to be the all group value but isn't really designed to work))
]]
-- switch the active mod group directly to a number (no cap)
function CALC:switchToGroup(num)
    if num == self.debugstate.activeModGroup then
        -- self.debugstate.activeModGroup = -1
    else
        self.debugstate.activeModGroup = num
    end
    self:announceActiveDebugMods()
end

--[[
    uhhh so this does the same as the immediately above thing
    yeah
]]
function CALC:switchToDiffGroup(num, steps)
    if num == self.debugstate.activeDiffGroup then
        -- self.debugstate.activeDiffGroup = -1
    else
        self.debugstate.activeDiffGroup = num
    end

    -- generate ssrs only if they are visible, but only once
    -- they get cleared on song change
    -- (it lags)
    if self.diffGroups[self.debugstate.activeDiffGroup]["SSRS"] then
        if #self.debugstate.ssrs == 0 and steps then
            self.debugstate.ssrs = self:getGraphForSteps(steps)
            MESSAGEMAN:Broadcast("UpdateSSRLines")
        end
    end

    MESSAGEMAN:Broadcast("UpdateActiveLowerGraph")
end

-- move the active mod group value in a direction (looping)
function CALC:addToModGroup(direction)
    if self.debugstate.activeModGroup == -1 then
        if direction < 0 then
            self:switchToGroup(#self.debugGroups)
        elseif direction > 0 then
            self:switchToGroup(1)
        end
    else
        local newg = (((self.debugstate.activeModGroup) + direction) % (#self.debugGroups + 1))
        if newg == 0 then
            newg = direction > 0 and 1 or #self.debugGroups
        end
        self:switchToGroup(newg)
    end
end

-- move the active diff group value in a direction (looping)
function CALC:addToDiffGroup(direction, steps)
    if self.debugstate.activeDiffGroup == -1 then
        if direction < 0 then
            self:switchToDiffGroup(#self.diffGroups, steps)
        elseif direction > 0 then
            self:switchToDiffGroup(1, steps)
        end
    else
        local newg = (((self.debugstate.activeDiffGroup) + direction) % (#self.diffGroups + 1))
        if newg == 0 then
            newg = direction > 0 and 1 or #self.diffGroups
        end
        self:switchToDiffGroup(newg, steps)
    end
end

-- this will gather all the mod names and values for a specific given index
-- it produces a single string for the purpose of hover information
function CALC:getDebugModsForIndex(modgroup, modgroupname, extramodgroup, index, isUpper)
    local modsToValues = {}
    local modText = ""
    local modNames = {}

    -- gather all mods and names and values and stuff to put them in the hover text
    for i, mod in pairs(modgroup) do
        local mod = shortenEnum(modgroupname, mod)
        for h = 1, 2 do
            local hand = h == 2 and "R" or "L"
            modsToValues[#modsToValues + 1] = self.debugstate.graphVecs[mod][h]
            modNames[#modNames + 1] = mod..hand
        end
    end

    -- for each hand add the enum-less mods (handle them last)
    for mod, _ in pairs(extramodgroup) do
        local mod = shortenEnum("CalcDebugMisc", mod)
        for h = 1,2 do
            local hand = h == 2 and "R" or "L"
            modsToValues[#modsToValues + 1] = self.debugstate.graphVecs[mod][h]
            modNames[#modNames + 1] = mod..hand
        end
    end
    
    -- carry out the final string production
    for k, v in pairs(modsToValues) do
        local namenoHand = modNames[k]:sub(1, #modNames[k]-1)
        if isUpper and (self.debugstate.activeModGroup == -1 or self.debugGroups[self.debugstate.activeModGroup][namenoHand]) then
            local name = modNames[k] and modNames[k] or ""
            local value = v[index] and v[index] or 0
            local txt = string.format(name..": %5.4f\n", value)
            modText = modText .. txt
        elseif not isUpper and (self.debugstate.activeDiffGroup == -1 or self.diffGroups[self.debugstate.activeDiffGroup][namenoHand]) then
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

