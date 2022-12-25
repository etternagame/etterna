local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())
local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats()
-- keep track of the current displayed score so we can refer back to it
local chosenScore
local mostRecentScore = SCOREMAN:GetMostRecentScore()
local screen
local usingCustomWindows = false

local t = Def.ActorFrame {
    Name = "MainDisplayFile",
    BeginCommand = function(self)
        screen = SCREENMAN:GetTopScreen()
        screen:AddInputCallback(function(event)
            if event.type == "InputEventType_FirstPress" then
                local btn = event.GameButton
                if btn ~= nil then
                    local dir = 0
                    if btn == "EffectUp" then
                        -- judge window increase
                        dir = 1
                    elseif btn == "EffectDown" then
                        -- judge window decrease
                        dir = -1
                    end

                    if dir ~= 0 and not usingCustomWindows then
                        judgeSetting = clamp(judgeSetting + dir, 4, 9)
                        MESSAGEMAN:Broadcast("JudgeWindowChanged")
                    elseif dir ~= 0 and usingCustomWindows then
                        self:playcommand("MoveCustomWindowIndex", {direction = dir})
                    end

                    if btn == "Coin" then
                        self:playcommand("ToggleCustomWindows")
                    end
                end
            end
        end)
        updateDiscordStatus(true)
    end,
    OnCommand = function(self)
        local score = SCOREMAN:GetMostRecentScore()
        if not score then
            score = SCOREMAN:GetTempReplayScore()
        end
        chosenScore = score

        -- use this to force J4 init for SSRNorm
        local forcedScreenEntryJudgeWindow = nil
        if PREFSMAN:GetPreference("SortBySSRNormPercent") then
            forcedScreenEntryJudgeWindow = 4
        end
        -- update replaysnapshots and pss for current score being rejudged to whatever judge
        screen:RescoreReplay(pss, ms.JudgeScalers[forcedScreenEntryJudgeWindow or judgeSetting], score, usingCustomWindows and currentCustomWindowConfigUsesOldestNoteFirst())

        --- propagate set command through children with the song
        self:playcommand("Set", {
            song = GAMESTATE:GetCurrentSong(),
            steps = GAMESTATE:GetCurrentSteps(),
            score = score,
            judgeSetting = forcedScreenEntryJudgeWindow
        })
    end,
    UpdateScoreCommand = function(self, params)
        -- update the global judge setting if it is provided and just in case it happens to be desynced here
        -- (it desyncs if picking scores, this will fix it)
        -- otherwise it should already be set by the input event
        if params.judgeSetting ~= nil then
            judgeSetting = params.judgeSetting
        end

        -- for custom windows we have to recalculate everything once to update normal values and then recalculate again for custom windows itself
        if usingCustomWindows and params.score ~= nil and chosenScore ~= nil and params.score ~= chosenScore then
            usingCustomWindows = false
            screen:RescoreReplay(pss, ms.JudgeScalers[judgeSetting], params.score, false)
            --- update all relevant information according to the given score
            -- should work with offset plot as well as all regular information on this screen
            -- this is intended for use only with replays but may partially work without it
            chosenScore = params.score
            self:playcommand("Set", {
                song = GAMESTATE:GetCurrentSong(),
                steps = GAMESTATE:GetCurrentSteps(),
                score = params.score,
                judgeSetting = params.judgeSetting,
                rejudged = params.rejudged, -- optional param to know if need to reload offset plot
                usingCustomWindows = false,
            })
            usingCustomWindows = true
        end

        -- we assume the score has a replay
        -- recalculate all stats using the replay
        screen:RescoreReplay(pss, ms.JudgeScalers[judgeSetting], params.score, usingCustomWindows and currentCustomWindowConfigUsesOldestNoteFirst())

        chosenScore = params.score

        --- update all relevant information according to the given score
        -- should work with offset plot as well as all regular information on this screen
        -- this is intended for use only with replays but may partially work without it
        self:playcommand("Set", {
            song = GAMESTATE:GetCurrentSong(),
            steps = GAMESTATE:GetCurrentSteps(),
            score = params.score,
            judgeSetting = params.judgeSetting,
            rejudged = params.rejudged, -- optional param to know if need to reload offset plot
            usingCustomWindows = usingCustomWindows,
        })
    end,
    JudgeWindowChangedMessageCommand = function(self)
        -- we assume a score is already set
        -- so we run it back through again
        -- the fact that the param table has judgeSetting in it causes things to recalc according to judge
        self:playcommand("UpdateScore", {
            score = chosenScore,
            judgeSetting = judgeSetting,
            rejudged = true,
        })
    end,
    ToggleCustomWindowsMessageCommand = function(self)
        usingCustomWindows = not usingCustomWindows

        if usingCustomWindows then
            loadCurrentCustomWindowConfig()
            screen:RescoreReplay(pss, 1, chosenScore, currentCustomWindowConfigUsesOldestNoteFirst())
            self:playcommand("Set", {
                song = GAMESTATE:GetCurrentSong(),
                steps = GAMESTATE:GetCurrentSteps(),
                score = chosenScore,
                judgeSetting = judgeSetting,
                rejudged = true,
                usingCustomWindows = usingCustomWindows,
            })
        else
            unloadCustomWindowConfig()
            self:playcommand("UpdateScore", {
                judgeSetting = judgeSetting,
                rejudged = true,
                score = chosenScore,
                usingCustomWindows = usingCustomWindows,
            })
        end

    end,
    MoveCustomWindowIndexMessageCommand = function(self, params)
        if not usingCustomWindows then return end

        moveCustomWindowConfigIndex(params.direction)
        loadCurrentCustomWindowConfig()
        screen:RescoreReplay(pss, 1, chosenScore, currentCustomWindowConfigUsesOldestNoteFirst())
        self:playcommand("Set", {
            song = GAMESTATE:GetCurrentSong(),
            steps = GAMESTATE:GetCurrentSteps(),
            score = chosenScore,
            judgeSetting = judgeSetting,
            rejudged = true,
            usingCustomWindows = usingCustomWindows,
        })

    end,
    EndCommand = function(self)
        unloadCustomWindowConfig()
    end,
    LoginMessageCommand = function(self)
        self:playcommand("UpdateLoginStatus")
    end,
    LogOutMessageCommand = function(self)
        self:playcommand("UpdateLoginStatus")
    end,
    LoginFailedMessageCommand = function(self)
        self:playcommand("UpdateLoginStatus")
    end,
    OnlineUpdateMessageCommand = function(self)
        self:playcommand("UpdateLoginStatus")
    end
}

local ratios = {
    LeftGap = 78 / 1920,
    UpperGap = 135 / 1080, -- from top edge of screen to edge of bg
    Width = 1765 / 1920,
    Height = 863 / 1080,
    LipLeftGap = 800 / 1920, -- the lip starts at the end of the banner
    LipHeight = 50 / 1080,
    LipLength = 965 / 1920, -- runs to the right end of the frame

    GraphLeftGap = 18 / 1920,
    GraphWidth = 739 / 1920, -- this must be the same as in metrics [GraphDisplay/ComboGraph]
    GraphBannerGap = 9 / 1080, -- from bottom of banner to top of graph
    BannerLeftGap = 18 / 1920,
    BannerUpperGap = 7 / 1080,
    BannerHeight = 228 / 1080,
    BannerWidth = 739 / 1920,
    LifeGraphHeight = 71 / 1080, -- this must be the same as in metrics [GraphDisplay]
    ComboGraphHeight = 16 / 1080, -- this must be the same as in metrics [ComboGraph]

    DividerThickness = 2 / 1080,
    LeftDividerLeftGap = 18 / 1920,
    LeftDividerLength = 739 / 1920,

    LeftDivider1UpperGap = 338 / 1080,
    LeftDivider2UpperGap = 399 / 1080,

    ModTextLeftGap = 19 / 1920,
    -- modtext Y pos is half between the 2 dividers.

    JudgmentBarLeftGap = 18 / 1920, -- edge of frame to left of bar
    JudgmentBarUpperGap = 408 / 1080, -- top edge of from to top edge of top bar
    JudgmentBarHeight = 44 / 1080,
    JudgmentBarAllottedSpace = 264 / 1080, -- top of top bar to top of bottom bar (valign 0)
    JudgmentBarLength = 739 / 1920,
    JudgmentBarSpacing = 7 / 1080, -- the emptiness between judgments
    JudgmentNameLeftGap = 25 / 1920, -- from left edge of bar to left edge of text
    JudgmentCountRightGap = 95 / 1920, -- from right edge of bar to left edge of percentage, right edge of count

    BottomTextUpperGap = 733 / 1080, -- top edge of frame to top edge of the text at the bottom left of the screen
    BottomTextHeight = 16 / 1080, -- fudge
    BottomTextSpacing = 10 / 1080, -- mix with the immediately above value
    SubTypeTextLeftGap = 18 / 1920, -- edge of frame to left of text
    SubTypeNumberCenter = 170 / 1920, -- from left edge of text to center of the /
    SubTypeNumberWidth = 145 / 1920, -- approximate width of the numbers including the /
    SubTypeAllottedSpace = 105 / 1080, -- top of top text to top of bottom text (valign 0)

    StatTextRightGap = 225 / 1920, -- right edge of stat count text to left edge of name text
    StatCountTotalRightGap = 0 / 1920, -- this is a base line, probably 0, here for consistency
    StatTextAllottedSpace = 105 / 1080, -- top of top text to top of bottom text (valign 0)

    RightHalfLeftGap = 803 / 1920, -- left edge of frame to left edge of everything on the right side
    RightHalfRightAlignLeftGap = 936 / 1920, -- basically the same length as the divider, right end of rightest right text
    RightHorizontalDividerLength = 936 / 1920,
    RightHorizontalDivider1UpperGap = 244 / 1080, -- top of frame to top of divider
    RightHorizontalDivider2UpperGap = 544 / 1080, -- same

    SongTitleLowerGap = 147 / 1080, -- top of divider to bottom of text
    SongArtistLowerGap = 103 / 1080, -- same
    SongPackLowerGap = 59 / 1080, -- ...
    SongRateLowerGap = 15 / 1080,
    GradeLowerGap = 141 / 1080,
    WifePercentLowerGap = 77 / 1080,
    MSDInfoLowerGap = 17 / 1080,

    ScoreBoardHeight = 298 / 1080, -- inner edge of divider to inner edge of divider

    OffsetPlotUpperGap = 559 / 1080, -- from top of frame to top of plot
    OffsetPlotHeight = 295 / 1080,
    OffsetPlotWidth = 936 / 1920,
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    UpperGap = ratios.UpperGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    LipLeftGap = ratios.LipLeftGap * SCREEN_WIDTH,
    LipHeight = ratios.LipHeight * SCREEN_HEIGHT,
    LipLength = ratios.LipLength * SCREEN_WIDTH,
    GraphLeftGap = ratios.GraphLeftGap * SCREEN_WIDTH,
    GraphWidth = ratios.GraphWidth * SCREEN_WIDTH,
    GraphBannerGap = ratios.GraphBannerGap * SCREEN_HEIGHT,
    BannerLeftGap = ratios.BannerLeftGap * SCREEN_WIDTH,
    BannerUpperGap = ratios.BannerUpperGap * SCREEN_HEIGHT,
    BannerHeight = ratios.BannerHeight * SCREEN_HEIGHT,
    BannerWidth = ratios.BannerWidth * SCREEN_WIDTH,
    LifeGraphHeight = ratios.LifeGraphHeight * SCREEN_HEIGHT,
    ComboGraphHeight = ratios.ComboGraphHeight * SCREEN_HEIGHT,
    DividerThickness = ratios.DividerThickness * SCREEN_HEIGHT,
    LeftDividerLeftGap = ratios.LeftDividerLeftGap * SCREEN_WIDTH,
    LeftDividerLength = ratios.LeftDividerLength * SCREEN_WIDTH,
    LeftDivider1UpperGap = ratios.LeftDivider1UpperGap * SCREEN_HEIGHT,
    LeftDivider2UpperGap = ratios.LeftDivider2UpperGap * SCREEN_HEIGHT,
    ModTextLeftGap = ratios.ModTextLeftGap * SCREEN_WIDTH,
    JudgmentBarLeftGap = ratios.JudgmentBarLeftGap * SCREEN_WIDTH,
    JudgmentBarUpperGap = ratios.JudgmentBarUpperGap * SCREEN_HEIGHT,
    JudgmentBarHeight = ratios.JudgmentBarHeight * SCREEN_HEIGHT,
    JudgmentBarAllottedSpace = ratios.JudgmentBarAllottedSpace * SCREEN_HEIGHT,
    JudgmentBarLength = ratios.JudgmentBarLength * SCREEN_WIDTH,
    JudgmentBarSpacing = ratios.JudgmentBarSpacing * SCREEN_HEIGHT,
    JudgmentNameLeftGap = ratios.JudgmentNameLeftGap * SCREEN_WIDTH,
    JudgmentCountRightGap = ratios.JudgmentCountRightGap * SCREEN_WIDTH,
    BottomTextUpperGap = ratios.BottomTextUpperGap * SCREEN_HEIGHT,
    BottomTextHeight = ratios.BottomTextHeight * SCREEN_HEIGHT,
    BottomTextSpacing = ratios.BottomTextSpacing * SCREEN_HEIGHT,
    SubTypeTextLeftGap = ratios.SubTypeTextLeftGap * SCREEN_WIDTH,
    SubTypeNumberCenter = ratios.SubTypeNumberCenter * SCREEN_WIDTH,
    SubTypeNumberWidth = ratios.SubTypeNumberWidth * SCREEN_WIDTH,
    SubTypeAllottedSpace = ratios.SubTypeAllottedSpace * SCREEN_HEIGHT,
    StatTextRightGap = ratios.StatTextRightGap * SCREEN_WIDTH,
    StatCountTotalRightGap = ratios.StatCountTotalRightGap * SCREEN_WIDTH,
    StatTextAllottedSpace = ratios.StatTextAllottedSpace * SCREEN_HEIGHT,
    RightHalfLeftGap = ratios.RightHalfLeftGap * SCREEN_WIDTH,
    RightHalfRightAlignLeftGap = ratios.RightHalfRightAlignLeftGap * SCREEN_WIDTH,
    RightHorizontalDividerLength = ratios.RightHorizontalDividerLength * SCREEN_WIDTH,
    RightHorizontalDivider1UpperGap = ratios.RightHorizontalDivider1UpperGap * SCREEN_HEIGHT,
    RightHorizontalDivider2UpperGap = ratios.RightHorizontalDivider2UpperGap * SCREEN_HEIGHT,
    SongTitleLowerGap = ratios.SongTitleLowerGap * SCREEN_HEIGHT,
    SongArtistLowerGap = ratios.SongArtistLowerGap * SCREEN_HEIGHT,
    SongPackLowerGap = ratios.SongPackLowerGap * SCREEN_HEIGHT,
    SongRateLowerGap = ratios.SongRateLowerGap * SCREEN_HEIGHT,
    GradeLowerGap = ratios.GradeLowerGap * SCREEN_HEIGHT,
    WifePercentLowerGap = ratios.WifePercentLowerGap * SCREEN_HEIGHT,
    MSDInfoLowerGap = ratios.MSDInfoLowerGap * SCREEN_HEIGHT,
    ScoreBoardHeight = ratios.ScoreBoardHeight * SCREEN_HEIGHT,
    OffsetPlotUpperGap = ratios.OffsetPlotUpperGap * SCREEN_HEIGHT,
    OffsetPlotHeight = ratios.OffsetPlotHeight * SCREEN_HEIGHT,
    OffsetPlotWidth = ratios.OffsetPlotWidth * SCREEN_WIDTH,
}

local translations = {
    Title = THEME:GetString("ScreenEvaluation", "Title"),
    ReplayTitle = THEME:GetString("ScreenEvaluation", "ReplayTitle"),
    Mean = THEME:GetString("ScreenEvaluation", "Mean"),
    StandardDeviation = THEME:GetString("ScreenEvaluation", "StandardDeviation"),
    Largest = THEME:GetString("ScreenEvaluation", "Largest"),
    LeftCBs = THEME:GetString("ScreenEvaluation", "LeftCBs"),
    MiddleCBs = THEME:GetString("ScreenEvaluation", "MiddleCBs"),
    RightCBs = THEME:GetString("ScreenEvaluation", "RightCBs"),
    CBsPerColumn = THEME:GetString("ScreenEvaluation", "CBsPerColumn"),
    Column = THEME:GetString("ScreenEvaluation", "Column"),
    Justice = THEME:GetString("ScreenEvaluation", "Justice"),
}

-- require that the banner ratio is 3.2 for consistency
local nonstandardBannerSizing = false
do
    local rat = actuals.BannerWidth / actuals.BannerHeight
    if rat ~= 3.2 then
        local possibleHeight = actuals.BannerWidth / 3.2
        if possibleHeight > actuals.BannerHeight then
            -- height stays, width moves
            actuals.BannerWidth = actuals.BannerHeight * 3.2
        else
            -- width stays, height moves
            actuals.BannerHeight = actuals.BannerWidth / 3.2
        end
        -- this will produce a visible gap but the ratio will stay the same
        nonstandardBannerSizing = true
    end
    actuals.BannerAreaHeight = ratios.BannerHeight * SCREEN_HEIGHT
end

-- constant list of judgments for rescoring purposes
-- for the most part this list is the same as the one below but remains separate just "in case"
local tapJudgments = {
    "TapNoteScore_W1",
    "TapNoteScore_W2",
    "TapNoteScore_W3",
    "TapNoteScore_W4",
    "TapNoteScore_W5",
    "TapNoteScore_Miss",
}

-- list of tap/hold subtypes to display the counts for
-- these are each a part of RadarCategory_x
local subTypesChosen = {
    "Holds",
    "Mines",
    "Rolls",
    "Lifts",
    "Fakes",
}

local modTextZoom = 0.6

local subTypeTextZoom = 0.75
local subTypeTextBump = 5 -- a bump in position added to the bottom left numbers for spacing

local statTextZoom = 0.75
local statTextSuffixZoom = 0.65
local accStatZoom = 0.75

local titleTextSize = 0.8
local songInfoTextSize = 0.63
local scoreInfoTextSize = 0.8

local textzoomFudge = 5

-- a helper to get the radar value for a score and fall back to playerstagestats if that fails
-- it tends to fail a lot...
local function gatherRadarValue(radar, score)
    local n = score:GetRadarValues():GetValue(radar)
    if n == -1 then
        return pss:GetRadarActual():GetValue(radar)
    end
    return n
end

-- construct a table that is passed to the wife rescoring function
local function gatherRescoreTableFromScore(score)
    local o = {}
    -- tap offsets
    o["dvt"] = score:GetOffsetVector()
    -- holds
    o["totalHolds"] = pss:GetRadarPossible():GetValue("RadarCategory_Holds") + pss:GetRadarPossible():GetValue("RadarCategory_Rolls")
    o["holdsHit"] = gatherRadarValue("RadarCategory_Holds", score) + gatherRadarValue("RadarCategory_Rolls", score)
    o["holdsMissed"] = o["totalHolds"] - o["holdsHit"]
    -- mines
    o["minesHit"] = pss:GetRadarPossible():GetValue("RadarCategory_Mines") - gatherRadarValue("RadarCategory_Mines", score)
    -- taps
    o["totalTaps"] = 0
    for _, j in ipairs(tapJudgments) do
        o["totalTaps"] = o["totalTaps"] + score:GetTapNoteScore(j)
    end
    return o
end

local function subTypeStats()
    local t = Def.ActorFrame {Name = "SubTypeParentFrame"}
    local function makeLine(i)
        local rdr = subTypesChosen[i]

        return Def.ActorFrame {
            Name = "SubTypeLine_"..i,
            InitCommand = function(self)
                self:y((actuals.SubTypeAllottedSpace / (#subTypesChosen - 1)) * (i-1))
            end,

            LoadFont("Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoom(subTypeTextZoom)
                    self:maxwidth((actuals.SubTypeNumberCenter - subTypeTextBump - actuals.SubTypeNumberWidth / 2) / subTypeTextZoom - textzoomFudge)
                    self:settext(THEME:GetString("RadarCategory", rdr))
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end
            },
            Def.RollingNumbers {
                Name = "Count",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:Load("RollingNumbersSlow3Leading")
                    self:halign(1):valign(0)
                    self:x(actuals.SubTypeNumberCenter - subTypeTextBump)
                    self:zoom(subTypeTextZoom)
                    self:maxwidth((actuals.SubTypeNumberWidth / 2 - subTypeTextBump) / subTypeTextZoom - textzoomFudge)
                    self:targetnumber(0)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self, params)
                    if params.score == nil then
                        self:targetnumber(0)
                        return
                    end
                    local num = gatherRadarValue(rdr, params.score)
                    self:targetnumber(num)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Slash",
                InitCommand = function(self)
                    -- when you want to do something in a really particular way and dont trust anything else to get it right
                    self:valign(0)
                    self:x(actuals.SubTypeNumberCenter)
                    self:zoom(subTypeTextZoom)
                    self:settext("/")
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end
            },
            Def.RollingNumbers {
                Name = "Total",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:Load("RollingNumbersSlow3Leading")
                    self:halign(0):valign(0)
                    self:x(actuals.SubTypeNumberCenter + subTypeTextBump)
                    self:zoom(subTypeTextZoom)
                    self:maxwidth((actuals.SubTypeNumberWidth / 2 - subTypeTextBump) / subTypeTextZoom - textzoomFudge)
                    self:targetnumber(0)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self, params)
                    if params.score == nil then
                        self:targetnumber(0)
                        return
                    end
                    local num = pss:GetRadarPossible():GetValue("RadarCategory_"..rdr)
                    self:targetnumber(num)
                end
            }
        }
    end
    for i = 1, #subTypesChosen do
        t[#t+1] = makeLine(i)
    end

    return t
end

local function accuracyStats()
    local statTypes = {
        "EvalRA", -- Ridiculous Attack - Ratio of J7 Marvelous to J7 Perfect
        "EvalMA", -- Marvelous Attack - Ratio of Marvelous to Perfects
        "EvalPA", -- Perfect Attack - Ratio of Perfects to Greats
        "EvalLongestMFC", -- Longest streak of Marvelous or better
        "EvalLongestPFC", -- Longest streak of Perfect or better
    }
    local statData = {
        0, -- Ridiculous divided by Marvelous
        0, -- Marvelous divided by Perfect
        0, -- Perfect divided by Great
        0, -- MFC length
        0, -- PFC length
    }

    -- BAD HACK TO FIT LONGEST RFC INTO THE EQUATION
    -- I HATE THIS
    local isMFC = false -- if true, Longest PFC becomes Longest RFC
    local rfcStatIndex = 5
    local rfcStatType = "EvalLongestRFC"
    local pfcStatType = "EvalLongestPFC"
    -- DONT DO THIS

    -- calculates the statData based on the given score
    local function calculateStatData(score)
        local replay = REPLAYS:GetActiveReplay()
        local offsetTable = usingCustomWindows and replay:GetOffsetVector() or score:GetOffsetVector()
        local typeTable = usingCustomWindows and replay:GetTapNoteTypeVector() or score:GetTapNoteTypeVector()

        -- must match statData above
        local output = {
            0, -- Ridiculous divided by Marvelous
            0, -- Marvelous and Ridiculous divided by Perfect (because we are used to that)
            0, -- Perfect divided by Great
            0, -- MFC length
            0, -- PFC length
        }

        if offsetTable == nil or #offsetTable == 0 or typeTable == nil or #typeTable == 0 then
            return output
        end

        local windowtable = usingCustomWindows and getCurrentCustomWindowConfigJudgmentWindowTable() or {
            TapNoteScore_W1 = 22.5 * ms.JudgeScalers[judgeSetting],
            TapNoteScore_W2 = 45 * ms.JudgeScalers[judgeSetting],
            TapNoteScore_W3 = 90 * ms.JudgeScalers[judgeSetting],
        }
        windowtable["TapNoteScore_W0"] = windowtable["TapNoteScore_W1"] / 2

        local ridicThreshold = windowtable["TapNoteScore_W0"] -- this is the J7 Marvelous window
        local marvThreshold = windowtable["TapNoteScore_W1"] -- J4 Marvelous window
        local perfThreshold = windowtable["TapNoteScore_W2"] -- J4 Perfect window
        local greatThreshold = windowtable["TapNoteScore_W3"] -- J4 Great window
        local currentRFC = 0
        local currentMFC = 0
        local currentPFC = 0
        local marvsForRA = 0 -- we are counting ridic as marvs normally, so count marvs alone separately to calculate RA
        local greatCount = 0
        local longestRFC = 0

        local validTaps = 0

        for i, o in ipairs(offsetTable) do
            if typeTable[i] ~= nil and (typeTable[i] == "TapNoteType_Tap" or typeTable[i] == "TapNoteType_HoldHead") then
                local off = math.abs(o)
                validTaps = validTaps + 1

                -- count judgments
                if off <= ridicThreshold then
                    currentRFC = currentRFC + 1
                    currentMFC = currentMFC + 1
                    currentPFC = currentPFC + 1
                    output[1] = output[1] + 1
                    output[2] = output[2] + 1
                elseif off <= marvThreshold then
                    currentRFC = 0
                    currentMFC = currentMFC + 1
                    currentPFC = currentPFC + 1
                    output[2] = output[2] + 1
                    marvsForRA = marvsForRA + 1
                elseif off <= perfThreshold then
                    currentRFC = 0
                    currentMFC = 0
                    currentPFC = currentPFC + 1
                    output[3] = output[3] + 1
                elseif off <= greatThreshold then
                    currentRFC = 0
                    currentMFC = 0
                    currentPFC = 0
                    greatCount = greatCount + 1
                else
                    -- worse than a great
                    currentMFC = 0
                    currentPFC = 0
                end

                -- set new highest MFC/PFC at end of iteration
                if currentMFC > output[4] then
                    output[4] = currentMFC
                end
                if currentPFC > output[5] then
                    output[5] = currentPFC
                end
                if currentRFC > longestRFC then
                    longestRFC = currentRFC
                end
            end
        end

        if output[4] == validTaps then
            output[rfcStatIndex] = longestRFC
            statTypes[rfcStatIndex] = rfcStatType 
        else
            statTypes[rfcStatIndex] = pfcStatType 
        end

        -- prevent division by 0 here
        if marvsForRA > 0 then
            output[1] = output[1] / marvsForRA
        else
            output[1] = -1
        end
        if output[3] > 0 then
            output[2] = output[2] / output[3]
        else
            output[2] = -1
        end
        if greatCount > 0 then
            output[3] = output[3] / greatCount
        else
            output[3] = -1
        end

        return output
    end

    local t = Def.ActorFrame {
        Name = "AccuracyStatsParentFrame",
        SetCommand = function(self, params)
            if params.steps ~= nil then
                -- this recalculates the stats to display for the following texts
                statData = calculateStatData(params.score)

                self:playcommand("UpdateStats", {score = params.score})
            end
        end
    }
    local function makeLine(i)
        return Def.ActorFrame {
            Name = "Stat_"..i,
            InitCommand = function(self)
                self:y((actuals.StatTextAllottedSpace / (#statTypes - 1)) * (i-1))
            end,
            Def.RollingNumbers {
                Name = "Number",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:Load("RollingNumbers" .. statTypes[i])
                    self:valign(0)
                    self:zoom(accStatZoom)
                    self:maxwidth((actuals.JudgmentBarLength - actuals.StatTextRightGap - actuals.SubTypeNumberCenter - subTypeTextBump - actuals.SubTypeTextLeftGap) / 1.25 / accStatZoom - textzoomFudge)
                    self:targetnumber(0)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                UpdateStatsCommand = function(self, params)
                    if statData[i] == -1 then
                        self:Load("RollingNumbers" .. statTypes[i] .. "INF")
                        self:targetnumber(99999)
                    else
                        self:Load("RollingNumbers" .. statTypes[i])
                        self:targetnumber(statData[i])
                    end
                end
            }
        }
    end

    for i = 1, #statTypes do
        t[#t+1] = makeLine(i)
    end
    return t
end

local function calculatedStats()
    -- list of stats
    -- do not allow this list to be shorter than 2 in length
    local statStrings = {
        translations["Mean"],
        translations["StandardDeviation"],
        translations["Largest"],
        translations["LeftCBs"],
        translations["MiddleCBs"], -- skip this index for even column types
        translations["RightCBs"],
    }

    -- RollingNumber types in metrics
    -- so we can assign it without so much work
    local statTypes = {
        "Slow2DecimalNoLeadMilliseconds",
        "Slow2DecimalNoLeadMilliseconds",
        "Slow2DecimalNoLeadMilliseconds",
        "SlowNoLead",
        "SlowNoLead",
        "SlowNoLead",
    }

    local evenColumns = true
    local indexToSkip = 5 -- the middle cb index

    -- contains the data corresponding to each of the above stat strings
    local statData = {
        0, -- mean
        0, -- sd
        0, -- largest deviation
        0, -- left cb
        0, -- middle cb
        0, -- right cb
    }

    local cbInfo = {0,0,0,0} -- per column cb info

    local function calculateStatData(score, numColumns)
        local replay = REPLAYS:GetActiveReplay()
        local tracks = usingCustomWindows and replay:GetTrackVector() or score:GetTrackVector()
        local offsetTable = usingCustomWindows and replay:GetOffsetVector() or score:GetOffsetVector()

        local middleColumn = numColumns / 2

        -- MUST MATCH statData above
        local output = {
            0, -- mean
            0, -- sd
            0, -- largest deviation
            0, -- left cb
            0, -- middle cb
            0, -- right cb
        }

        local cbInfo = {}
        for _ = 1, numColumns + 1 do
            cbInfo[#cbInfo+1] = 0
        end

        if offsetTable == nil or #offsetTable == 0 then
            return output, cbInfo
        end


        local cbThreshold = usingCustomWindows and getCurrentCustomWindowConfigJudgmentWindowTable()["TapNoteScore_W3"] or (ms.JudgeScalers[judgeSetting] * 90)
        local leftCB = 0
        local middleCB = 0
        local rightCB = 0

        -- count CBs
        for i, o in ipairs(offsetTable) do
            -- online replays return 180 instead of "1000" for misses
            if o == 180 then
                offsetTable[i] = 1000
                o = 1000
            end
            if tracks[i] then
                if math.abs(o) > cbThreshold then
                    if tracks[i] < middleColumn then
                        leftCB = leftCB + 1
                    elseif tracks[i] > middleColumn then
                        rightCB = rightCB + 1
                    else
                        middleCB = middleCB + 1
                    end

                    cbInfo[tracks[i]+1] = cbInfo[tracks[i]+1] + 1
                end
            end
        end

        local smallest, largest = wifeRange(offsetTable)

        -- MUST MATCH statData above
        output = {
            wifeMean(offsetTable), -- mean
            wifeSd(offsetTable), -- sd
            largest,
            leftCB,
            middleCB,
            rightCB,
        }
        return output, cbInfo
    end

    local t = Def.ActorFrame {
        Name = "CalculatedStatsParentFrame",
        SetCommand = function(self, params)
            if params.steps ~= nil then
                if params.steps:GetNumColumns() % 2 ~= 0 then
                    evenColumns = false
                end
                -- this recalculates the stats to display for the following texts
                -- subtract 1 from the number of columns because we are indexing at 0 in some of the data
                -- and it produces the numbers we want
                statData, cbInfo = calculateStatData(params.score, params.steps:GetNumColumns() - 1)

                self:playcommand("UpdateStats", {score = params.score})
            end
        end
    }
    local function makeLine(i)
        local statname = statStrings[i]
        return Def.ActorFrame {
            Name = "Stat_"..i,
            InitCommand = function(self)
                self:y((actuals.StatTextAllottedSpace / (#statStrings - 1)) * (i-1))
            end,
            UpdateStatsCommand = function(self, params)
                if evenColumns and i == indexToSkip then
                    self:diffusealpha(0)
                else
                    self:diffusealpha(1)
                end
                if evenColumns then
                    if i ~= indexToSkip then
                        -- this will convert the index to either i or i-1
                        -- because when we skip an index we want to place it as if nothing changed
                        -- and we are using a slightly shorter range than usual anyways
                        local j = (i < indexToSkip and i or i-1)
                        self:y((actuals.StatTextAllottedSpace / (#statStrings - 2)) * (j-1))
                    end
                end
            end,

            LoadFont("Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoom(statTextZoom)
                    self:maxwidth(actuals.StatTextRightGap / 2 / statTextZoom - textzoomFudge)
                    self:settext(statname)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end
            },
            Def.RollingNumbers {
                Name = "Number",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:Load("RollingNumbers" .. statTypes[i])
                    self:halign(1):valign(0)
                    -- note to self make this name less confusing
                    self:x(actuals.StatTextRightGap)
                    self:zoom(statTextZoom)
                    -- no fudge
                    self:maxwidth(actuals.StatTextRightGap / 2 / statTextZoom)
                    self:targetnumber(0)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                UpdateStatsCommand = function(self, params)
                    self:targetnumber(statData[i])
                end
            }
        }

    end

    for i = 1, #statStrings do
        t[#t+1] = makeLine(i)
    end

    -- displays some extra stats on hover
    t[#t+1] = UIElements.QuadButton(1) .. {
        Name = "StatMouseHoverBox",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:x(actuals.StatTextRightGap)
            self:zoomto(actuals.JudgmentBarLength, actuals.StatTextAllottedSpace / (#statStrings - 1) * #statStrings)
            self:diffusealpha(0)
        end,
        MouseOverCommand = function(self) self:playcommand("RolloverUpdate",{update = "over"}) end,
        MouseOutCommand = function(self) self:playcommand("RolloverUpdate",{update = "out"}) end,
        RolloverUpdateCommand = function(self, params)
            -- hovering
            if params.update == "over" then
                local cbstr = {translations["CBsPerColumn"], "\n"}
                for _ = 1, #cbInfo do
                    cbstr[#cbstr+1] = string.format("%s %d: %d", translations["Column"], _, cbInfo[_])
                    cbstr[#cbstr+1] = "\n"
                end
                cbstr[#cbstr] = nil
                cbstr = table.concat(cbstr)

                TOOLTIP:SetText(cbstr)
                TOOLTIP:Show()
            elseif params.update == "out" then
                TOOLTIP:Hide()
            end
        end,
    }

    return t
end

local function wifePercentDisplay()
    -- internal value storage defaults
    local value = 0
    local decimals = 2
    local stringformat = "%05."..decimals.."f%% [%s]"

    -- """constants"""
    local nothoverdecimals = 2 -- when not hovering
    local hoverdecimals = 4 -- when hovering
    local infostr = "" -- example: W3 J4

    return UIElements.TextToolTip(1, 1, "Common Large") .. {
        Name = "WifePercent",
        InitCommand = function(self)
            self:halign(1):valign(1)
            self:zoom(scoreInfoTextSize)
            self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / scoreInfoTextSize - textzoomFudge)
        end,
        UpdateParametersCommand = function(self, params)
            if params.decimals ~= nil then
                decimals = params.decimals
                stringformat = "%05."..decimals.."f%% [%s]"
            end
            if params.infostr ~= nil then
                infostr = params.infostr
            end
            if params.value ~= nil then
                value = params.value
            end
        end,
        UpdateTextCommand = function(self)
            self:settextf(stringformat, notShit.floor(value, decimals), infostr)
        end,
        ColorConfigUpdatedMessageCommand = function(self)
            self:playcommand("Set")
        end,
        SetCommand = function(self, params)
            if params.score ~= nil then
                if usingCustomWindows then return end
                local ver = params.score:GetWifeVers()
                local percent = params.score:GetWifeScore() * 100
                decimals = 2
                if params.judgeSetting ~= nil then
                    local rescoreTable = gatherRescoreTableFromScore(params.score)
                    percent = getRescoredWife3Judge(3, params.judgeSetting, rescoreTable)
                    ver = 3
                end
                -- wife version string
                local ws = "W"..ver.." "
                ws = ws .. (judgeSetting ~= 9 and "J"..judgeSetting or translations["Justice"])
                -- scores over 99% should show more decimals
                if percent > 99 or isOver(self) then
                    decimals = 4
                end
                local pg = notShit.floor(percent, decimals)
                local grade = GetGradeFromPercent(pg / 100)
                self:diffuse(colorByGrade(grade))
                self:playcommand("UpdateParameters", {decimals = decimals, infostr = ws, value = percent})
                self:playcommand("UpdateText")
            else
                self:settext("")
            end
        end,
        MouseOverCommand = function(self) self:playcommand("RolloverUpdate",{update = "over"}) end,
        MouseOutCommand = function(self) self:playcommand("RolloverUpdate",{update = "out"}) end,
        RolloverUpdateCommand = function(self, params)
            -- hovering
            if params.update == "over" then
                self:playcommand("UpdateParameters", {decimals = hoverdecimals})
            elseif params.update == "out" then
                self:playcommand("UpdateParameters", {decimals = nothoverdecimals})
            end
            self:playcommand("UpdateText")
        end,
        MouseDownCommand = function(self)
            MESSAGEMAN:Broadcast("ToggleCustomWindows")
        end,
    }
end

local function customScoringDisplay()
    -- internal value storage defaults
    local value = 0
    local decimals = 2
    local stringformat = "%05."..decimals.."f%% [%s]"

    -- """constants"""
    local nothoverdecimals = 2 -- when not hovering
    local hoverdecimals = 4 -- when hovering
    local infostr = "" -- example: W3 J4

    return UIElements.TextToolTip(1, 1, "Common Large") .. {
        Name = "CustomPercent",
        InitCommand = function(self)
            self:halign(1):valign(1)
            self:zoom(scoreInfoTextSize)
            self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / scoreInfoTextSize - textzoomFudge)
        end,
        UpdateParametersCommand = function(self, params)
            if params.decimals ~= nil then
                decimals = params.decimals
                stringformat = "%05."..decimals.."f%% [%s]"
            end
            if params.infostr ~= nil then
                infostr = params.infostr
            end
            if params.value ~= nil then
                value = params.value
            end
        end,
        UpdateTextCommand = function(self)
            self:settextf(stringformat, notShit.floor(value, decimals), infostr)
        end,
        ColorConfigUpdatedMessageCommand = function(self)
            self:playcommand("Set")
        end,
        SetCommand = function(self, params)
            if params.score ~= nil then
                
                self:visible(usingCustomWindows)
                if not usingCustomWindows then return end

                local lastSnapshot = REPLAYS:GetActiveReplay():GetLastReplaySnapshot()
                local percent = lastSnapshot:GetWifePercent() * 100
                local ss = getCurrentCustomWindowConfigName()

                -- scores over 99% should show more decimals
                if percent > 99 or isOver(self) then
                    decimals = 4
                end
                local pg = notShit.floor(percent, decimals)
                local grade = GetGradeFromPercent(pg / 100)
                self:diffuse(colorByGrade(grade))
                self:playcommand("UpdateParameters", {decimals = decimals, infostr = ss, value = percent})
                self:playcommand("UpdateText")
            else
                self:settext("")
            end
        end,
        MouseOverCommand = function(self) self:playcommand("RolloverUpdate",{update = "over"}) end,
        MouseOutCommand = function(self) self:playcommand("RolloverUpdate",{update = "out"}) end,
        RolloverUpdateCommand = function(self, params)
            -- hovering
            if params.update == "over" then
                self:playcommand("UpdateParameters", {decimals = hoverdecimals})
            elseif params.update == "out" then
                self:playcommand("UpdateParameters", {decimals = nothoverdecimals})
            end
            self:playcommand("UpdateText")
        end,
        MouseDownCommand = function(self, params)
            if self:IsInvisible() then return end
            MESSAGEMAN:Broadcast("MoveCustomWindowIndex", {direction = params.event == "DeviceButton_left mouse button" and 1 or -1})
        end,
    }
end

t[#t+1] = Def.ActorFrame {
    Name = "OwnerFrame",
    InitCommand = function(self)
        self:xy(actuals.LeftGap, actuals.UpperGap)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.Width, actuals.Height)
            self:diffusealpha(0.75)
            registerActorToColorConfigElement(self, "main", "PrimaryBackground")
        end
    },
    Def.Quad {
        Name = "BGLip",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:x(actuals.LipLeftGap)
            self:zoomto(actuals.LipLength, actuals.LipHeight)
            self:diffusealpha(1)
            registerActorToColorConfigElement(self, "main", "SecondaryBackground")
        end
    },
    Def.Quad {
        Name = "LeftUpperDivider",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.LeftDividerLength, actuals.DividerThickness)
            self:xy(actuals.LeftDividerLeftGap, actuals.LeftDivider1UpperGap)
            registerActorToColorConfigElement(self, "main", "SeparationDivider")
        end
    },
    Def.Quad {
        Name = "LeftLowerDivider",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.LeftDividerLength, actuals.DividerThickness)
            self:xy(actuals.LeftDividerLeftGap, actuals.LeftDivider2UpperGap)
            registerActorToColorConfigElement(self, "main", "SeparationDivider")
        end
    },
    Def.Quad {
        Name = "RightUpperHorizontalDivider",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.RightHorizontalDividerLength, actuals.DividerThickness)
            self:xy(actuals.RightHalfLeftGap, actuals.RightHorizontalDivider1UpperGap)
            registerActorToColorConfigElement(self, "main", "SeparationDivider")
        end
    },
    Def.Quad {
        Name = "RightLowerHorizontalDivider",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:zoomto(actuals.RightHorizontalDividerLength, actuals.DividerThickness)
            self:xy(actuals.RightHalfLeftGap, actuals.RightHorizontalDivider2UpperGap)
            registerActorToColorConfigElement(self, "main", "SeparationDivider")
        end
    },

    Def.Sprite {
        Name = "Banner",
        InitCommand = function(self)
            self:xy(actuals.BannerLeftGap, actuals.BannerUpperGap)
            self:valign(0):halign(0)
            self:scaletoclipped(actuals.BannerWidth, actuals.BannerHeight)
            if nonstandardBannerSizing then
                -- when the banner has been resized to an unexpected size, to fit the 3.2 ratio, reposition it
                -- this movement centers it in the area provided
                self:addy((actuals.BannerAreaHeight - actuals.BannerHeight) / 2)
            end
        end,
        SetCommand = function(self, params)
            self:finishtweening()
            self:smooth(0.05)
            self:diffusealpha(1)
            if params.song then
                local bnpath = params.song:GetBannerPath()
                if not showBanners() then
                    self:visible(false)
                elseif not bnpath  then
                    bnpath = THEME:GetPathG("Common", "fallback banner")
                    self:visible(false)
                else
                    self:visible(true)
                end
                self:LoadBackground(bnpath)
            else
                self:visible(false)
            end
        end
    },
    Def.GraphDisplay {
        Name = "LifeGraph",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:xy(actuals.GraphLeftGap, actuals.BannerUpperGap + actuals.GraphBannerGap + actuals.BannerHeight)
            if nonstandardBannerSizing then
                -- when the banner has been resized to an unexpected size, to fit the 3.2 ratio, reposition it
                -- this movement centers it in the area provided
                self:addy((actuals.BannerAreaHeight - actuals.BannerHeight) / 2)
            end

            -- due to reasons, the sizing for this is in metrics [GraphDisplay]
            -- we override them with the following zoomto
            -- so the ones in metrics can be anything....
            -- i don't like that
            self:Load("GraphDisplay")
            self:zoomto(actuals.GraphWidth, actuals.LifeGraphHeight)
            registerActorToColorConfigElement(self, "evaluation", "LifeGraphTint")

            -- hide the max life line and its dots (why does this exist?)
            self:GetChild("Line"):diffusealpha(0)
        end,
        SetCommand = function(self, params)
            if params.song ~= nil then
                self:SetWithoutStageStats(pss, params.song:GetStepsSeconds() / params.score:GetMusicRate())
            end
        end
    },
    Def.ComboGraph {
        Name = "ComboGraph",
        InitCommand = function(self)
            self:valign(0):halign(0)
            self:xy(actuals.GraphLeftGap, actuals.BannerUpperGap + actuals.GraphBannerGap + actuals.BannerHeight + actuals.LifeGraphHeight)
            if nonstandardBannerSizing then
                -- when the banner has been resized to an unexpected size, to fit the 3.2 ratio, reposition it
                -- this movement centers it in the area provided
                self:addy((actuals.BannerAreaHeight - actuals.BannerHeight) / 2)
            end

            -- due to reasons, the sizing for this is in metrics [ComboGraph]
            -- we dont override them here because the combo text is broken by the zoom
            -- self:zoomto(actuals.GraphWidth, actuals.ComboGraphHeight)
            registerActorToColorConfigElement(self, "evaluation", "ComboGraphTint")
        end,
        SetCommand = function(self, params)
            self:Clear()
            self:Load("ComboGraph")
            if params.song ~= nil then
                self:SetWithoutStageStats(pss, params.song:GetStepsSeconds() / params.score:GetMusicRate())
            end
        end
    },
    LoadFont("Common Large") .. {
        Name = "ModString",
        InitCommand = function(self)
            -- should be the upper divider + half the space between (accounting for the width of the top divider)
            local yPos = actuals.LeftDivider1UpperGap + (actuals.LeftDivider2UpperGap - actuals.LeftDivider1UpperGap) / 2 + actuals.DividerThickness
            self:xy(actuals.ModTextLeftGap, yPos)
            self:halign(0)
            self:zoom(modTextZoom)
            self:maxwidth(actuals.BannerWidth / modTextZoom - textzoomFudge)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            local mstr = params.score:GetModifiers()
            local ss = screen:GetStageStats()
            if not ss:GetLivePlay() then
                mstr = screen:GetReplayModifiers()
            end
            self:settext(getModifierTranslations(mstr))
        end
    },
    LoadActorWithParams("../judgmentBars", {
        sizing = {
            JudgmentBarHeight = actuals.JudgmentBarHeight,
            JudgmentBarLength = actuals.JudgmentBarLength,
            JudgmentBarSpacing = actuals.JudgmentBarSpacing,
            JudgmentBarAllottedSpace = actuals.JudgmentBarAllottedSpace,
            JudgmentNameLeftGap = actuals.JudgmentNameLeftGap,
            JudgmentCountRightGap = actuals.JudgmentCountRightGap,
        }}) .. {
        InitCommand = function(self)
            self:xy(actuals.JudgmentBarLeftGap, actuals.JudgmentBarUpperGap)
        end
    },
    subTypeStats() .. {
        InitCommand = function(self)
            self:xy(actuals.SubTypeTextLeftGap, actuals.BottomTextUpperGap)
        end
    },
    accuracyStats() .. {
        InitCommand = function(self)
            self:xy(actuals.JudgmentBarLeftGap + actuals.JudgmentBarLength / 2, actuals.BottomTextUpperGap)
        end
    },
    calculatedStats() .. {
        InitCommand = function(self)
            self:xy(actuals.JudgmentBarLeftGap + actuals.JudgmentBarLength - actuals.StatTextRightGap, actuals.BottomTextUpperGap)
        end
    },

    LoadFont("Common Large") .. {
        Name = "ScreenTitle",
        InitCommand = function(self)
            self:xy(actuals.RightHalfLeftGap, actuals.LipHeight / 2)
            self:halign(0)
            self:zoom(titleTextSize)
            self:maxwidth(actuals.OffsetPlotWidth / titleTextSize - textzoomFudge)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
            if GAMESTATE:GetCurrentSteps() ~= nil then
                local st = THEME:GetString("StepsDisplay StepsType", ToEnumShortString(GAMESTATE:GetCurrentSteps():GetStepsType()))
                self:settextf("%s %s", st, translations["Title"])
            else
                self:settext(translations["Title"])
            end
        end,
        SetCommand = function(self, params)
            if params.score then
                -- include stepstype in the results string
                local stStr = ""
                if params.steps then
                    stStr = THEME:GetString("StepsDisplay StepsType", ToEnumShortString(params.steps:GetStepsType())) .. " "
                end

                -- only the most recent score can cause this
                -- but view eval/replays also are mostrecentscores, so check the name
                -- the name is set for scores set during this session
                if mostRecentScore and params.score:GetName() == "#P1#" and params.score:GetScoreKey() == mostRecentScore:GetScoreKey() then
                    self:settext(stStr..translations["Title"])
                else
                    self:settext(stStr..translations["ReplayTitle"])
                end
            end
        end
    },
    Def.ActorFrame {
        Name = "BasicSongInfo",
        InitCommand = function(self)
            -- children y pos relative to the divider
            -- dont get confused
            self:xy(actuals.RightHalfLeftGap, actuals.RightHorizontalDivider1UpperGap)
        end,

        LoadFont("Common Large") .. {
            Name = "SongTitle",
            InitCommand = function(self)
                self:halign(0):valign(1)
                self:y(-actuals.SongTitleLowerGap)
                self:zoom(songInfoTextSize)
                -- allow 3/4 the width of the area
                self:maxwidth(actuals.RightHalfRightAlignLeftGap / 4 * 3 / songInfoTextSize - textzoomFudge)
                self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                if usingCustomWindows then
                    self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / songInfoTextSize - textzoomFudge)
                else
                    self:maxwidth(actuals.RightHalfRightAlignLeftGap / 4 * 3 / songInfoTextSize - textzoomFudge)
                end
            end,
        },
        LoadFont("Common Large") .. {
            Name = "SongArtist",
            InitCommand = function(self)
                self:halign(0):valign(1)
                self:y(-actuals.SongArtistLowerGap)
                self:zoom(songInfoTextSize)
                self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / songInfoTextSize - textzoomFudge)
                self:settext(GAMESTATE:GetCurrentSong():GetDisplayArtist())
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end
        },
        LoadFont("Common Large") .. {
            Name = "SongPack",
            InitCommand = function(self)
                self:halign(0):valign(1)
                self:y(-actuals.SongPackLowerGap)
                self:zoom(songInfoTextSize)
                self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / songInfoTextSize - textzoomFudge)
                self:settext(GAMESTATE:GetCurrentSong():GetGroupName())
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end
        },
        LoadFont("Common Large") .. {
            Name = "SongRate",
            InitCommand = function(self)
                self:halign(0):valign(1)
                self:y(-actuals.SongRateLowerGap)
                self:zoom(songInfoTextSize)
                self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / songInfoTextSize - textzoomFudge)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self, params)
                if params.score then
                    local r = params.score:GetMusicRate()
                    local rstr = getRateString(r)
                    self:settext(rstr)
                end
            end
        },
    },
    Def.ActorFrame {
        Name = "ScoreSpecificInfo",
        InitCommand = function(self)
            -- children y pos relative to the divider
            -- dont get confused
            self:xy(actuals.RightHalfLeftGap + actuals.RightHalfRightAlignLeftGap, actuals.RightHorizontalDivider1UpperGap)
        end,

        LoadFont("Common Large") .. {
            Name = "Grade",
            InitCommand = function(self)
                self:halign(1):valign(1)
                self:y(-actuals.GradeLowerGap)
                self:zoom(scoreInfoTextSize)
                -- allow 1/4 of the area (opposite of the title maxwidth)
                self:maxwidth(actuals.RightHalfRightAlignLeftGap / 4 / scoreInfoTextSize - textzoomFudge)
            end,
            SetCommand = function(self, params)
                self:visible(not usingCustomWindows)
                if usingCustomWindows then return end

                if params.score ~= nil then
                    local percent = params.score:GetWifeScore() * 100
                    if params.judgeSetting ~= nil then
                        local rescoreTable = gatherRescoreTableFromScore(params.score)
                        percent = getRescoredWife3Judge(3, params.judgeSetting, rescoreTable)
                    end
                    local grade = GetGradeFromPercent(percent / 100)

                    local gra = THEME:GetString("Grade", ToEnumShortString(grade))
                    self:settext(gra)
                    self:diffuse(colorByGrade(grade))
                else
                    self:settext("")
                end
            end,
        },

        -- this replaces the grade when custom window scoring is turned on
        -- by default it is turned off
        customScoringDisplay() .. {
            InitCommand = function(self)
                self:y(-actuals.GradeLowerGap)
                self:visible(false)
            end
        },

        -- to allow hovering for higher precision, wrap this in a function
        -- we use the function to store the numbers in order to change precision on the fly
        -- and that way we can store any number from any source and just change precision on it
        -- (only because i dont want to make the variables for it global: it just feels cleaner that way)
        wifePercentDisplay() .. {
            InitCommand = function(self)
                self:y(-actuals.WifePercentLowerGap)
            end
        },

        LoadFont("Common Large") .. {
            Name = "MSDSSRDiff",
            InitCommand = function(self)
                self:halign(1):valign(1)
                self:y(-actuals.MSDInfoLowerGap)
                self:zoom(scoreInfoTextSize)
                self:maxwidth(actuals.RightHalfRightAlignLeftGap / 2 / scoreInfoTextSize - textzoomFudge)
            end,
            SetCommand = function(self, params)
                if params.steps ~= nil then
                    local msd = params.steps:GetMSD(params.score:GetMusicRate(), 1)
                    local msdstr = string.format("%5.2f", msd)
                    local diff = getShortDifficulty(getDifficulty(params.steps:GetDifficulty()))
                    local diffcolor = colorByDifficulty(GetCustomDifficulty(params.steps:GetStepsType(), params.steps:GetDifficulty()))
                    local ssr = params.score:GetSkillsetSSR("Overall")
                    local ssrstr = string.format("%5.2f", ssr)
                    self:settextf("%s  ~  %s %s", msdstr, ssrstr, diff)
                    self:ClearAttributes()
                    self:diffuse(COLORS:getMainColor("PrimaryText"))
                    self:diffusealpha(1)
                    self:AddAttribute(0, {Length = #msdstr, Zoom = scoreInfoTextSize, Diffuse = colorByMSD(msd)})
                    self:AddAttribute(#msdstr + #"  ~  ", {Length = #ssrstr, Zoom = scoreInfoTextSize, Diffuse = colorByMSD(tonumber(ssrstr))})
                    self:AddAttribute(#msdstr + #"  ~  " + #ssrstr, {Length = -1, Zoom = scoreInfoTextSize, Diffuse = diffcolor})
                else
                    self:settext("")
                end
            end
        }
    },
    LoadActorWithParams("scoreBoard.lua", {
        Width = actuals.RightHorizontalDividerLength,
        Height = actuals.ScoreBoardHeight,
        DividerThickness = actuals.DividerThickness,
        ItemCount = 4,
        }) .. {
        InitCommand = function(self)
            self:xy(actuals.RightHalfLeftGap, actuals.RightHorizontalDivider1UpperGap + actuals.DividerThickness)
        end
    },
    LoadActorWithParams("../offsetplot.lua", {sizing = {Width = actuals.OffsetPlotWidth, Height = actuals.OffsetPlotHeight}, extraFeatures = true}) .. {
        InitCommand = function(self)
            self:xy(actuals.RightHalfLeftGap, actuals.OffsetPlotUpperGap)
        end,
        SetCommand = function(self, params)
            if params.score ~= nil and params.steps ~= nil then
                if params.score:HasReplayData() then
                    local replay = REPLAYS:GetActiveReplay()
                    local offsets = usingCustomWindows and replay:GetOffsetVector() or params.score:GetOffsetVector()
                    -- for online offset vectors a 180 offset is a miss
                    for i, o in ipairs(offsets) do
                        if o >= 180 then
                            offsets[i] = 1000
                        end
                    end
                    local tracks = usingCustomWindows and replay:GetTrackVector() or params.score:GetTrackVector()
                    local types = usingCustomWindows and replay:GetTapNoteTypeVector() or params.score:GetTapNoteTypeVector()
                    local noterows = usingCustomWindows and replay:GetNoteRowVector() or params.score:GetNoteRowVector()
                    local holds = usingCustomWindows and replay:GetHoldNoteVector() or params.score:GetHoldNoteVector()
                    local timingdata = params.steps:GetTimingData()
                    local lastSecond = params.steps:GetLastSecond()

                    self:playcommand("LoadOffsets", {
                        offsetVector = offsets,
                        trackVector = tracks,
                        timingData = timingdata,
                        noteRowVector = noterows,
                        typeVector = types,
                        holdVector = holds,
                        maxTime = lastSecond,
                        judgeSetting = params.judgeSetting,
                        columns = params.steps:GetNumColumns(),
                        rejudged = params.rejudged,
                        usingCustomWindows = usingCustomWindows,
                    })
                end
            end
        end
    }
}

return t
