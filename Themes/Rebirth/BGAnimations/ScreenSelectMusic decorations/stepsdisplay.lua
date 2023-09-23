local itsOn = false
local thesteps
local numshown = 5
local currentindex = 1
local displayindexoffset = 0

local ratios = {
    DiffItemWidth = 60 / 1920,
    DiffItemHeight = 40 / 1080,
    DiffFrameUpperGap = 282 / 1080, -- from frame top edge to top diff edge
    --DiffFrameLeftGap = 407 / 1920, -- this number is provided by the parent at this time
    --DiffFrameRightGap = 22 / 1920, -- same
    DiffFrameSpacing = 11 / 1920, -- spacing between items
    DiffItemGlowVerticalSpan = 14 / 1080, -- measurement of the visible portion of the glow, doubled
    DiffItemGlowHorizontalSpan = 14 / 1920, -- same as above
}

local actuals = {
    DiffItemWidth = ratios.DiffItemWidth * SCREEN_WIDTH,
    DiffItemHeight = ratios.DiffItemHeight * SCREEN_HEIGHT,
    DiffFrameUpperGap = ratios.DiffFrameUpperGap * SCREEN_HEIGHT,
    --DiffFrameLeftGap = ratios.DiffFrameLeftGap * SCREEN_WIDTH, -- this number is provided by the parent at this time
    --DiffFrameRightGap = ratios.DiffFrameRightGap * SCREEN_WIDTH, -- same
    DiffFrameSpacing = ratios.DiffFrameSpacing * SCREEN_WIDTH,
    DiffItemGlowVerticalSpan = ratios.DiffItemGlowVerticalSpan * SCREEN_HEIGHT,
    DiffItemGlowHorizontalSpan = ratios.DiffItemGlowHorizontalSpan * SCREEN_WIDTH,
}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

local textSize = 0.75
local textzoomFudge = 5
local hackyMaxWidth = Var("hackyMaxWidth") or false

-- this will return an index which is offset depending on certain conditions
-- basically we want the difficulties to be aligned to the right of the box
-- highest on the right
-- the highest diff is the highest index, but the highest index is not a consistent number
-- to avoid update order shenanigans we can do this math and logic instead
local function pushIndexByBound(index)
    if #thesteps < numshown then
        return index - numshown + #thesteps
    else
        return index
    end
end

-- based on the amount of difficulties displayed we can allow more room for the song information
-- assuming that we are aligning difficulties to the right
local function setMaxWidthForSongInfo()
    local curSongBox = SCREENMAN:GetTopScreen():safeGetChild("RightFrame", "CurSongBoxFile")
    if not curSongBox then return end
    if not hackyMaxWidth then return end

    local diffSlotsOpen = clamp(numshown - #thesteps, 0, numshown)
    -- exactly the width of <diffSlotsOpen> items including the space between plus an additional 2 gaps worth of space for buffer
    local widthallowed = actuals.DiffFrameLeftGap - actuals.LeftTextLeftGap + diffSlotsOpen * (actuals.DiffItemWidth + actuals.DiffFrameSpacing) - actuals.DiffFrameSpacing * 2

    local title = curSongBox:GetChild("Frame"):GetChild("TitleAuthor")
    local subtitle = curSongBox:GetChild("Frame"):GetChild("SubTitle")

    if title then
        title:maxwidth(widthallowed / title:GetZoom() - textzoomFudge)
    end

    if subtitle then
        subtitle:maxwidth(widthallowed / subtitle:GetZoom() - textzoomFudge)
    end

end

local t = Def.ActorFrame {
    Name = "StepsDisplayFile",
    InitCommand = function(self)
        -- all positions are relative to the right of the rightmost item
        -- align everything to the right from there
        self:xy(actuals.Width - actuals.DiffFrameRightGap, actuals.DiffFrameUpperGap)
    end,
    BeginCommand = function(self)
        local scrn = SCREENMAN:GetTopScreen()
        local snm = scrn:GetName()
        local anm = self:GetName()
        CONTEXTMAN:RegisterToContextSet(snm, "Main1", anm)
        -- input handling for changing difficulty with keyboard
        -- this timeout is basically just a timer to make sure that you press buttons fast enough
        local comboTimeout = nil
        local comboTimeoutSeconds = 1
        local pressqueue = {}

        local function clearTimeout()
            if comboTimeout ~= nil then
                scrn:clearInterval(comboTimeout)
                comboTimeout = nil
            end
        end

        local function resetTimeout()
            clearTimeout()
            -- we use setInterval because setTimeout doesnt do what I actually want
            -- wow very intuitive
            comboTimeout = scrn:setInterval(function()
                pressqueue = {}
                clearTimeout()
            end,
            comboTimeoutSeconds)
        end

        scrn:AddInputCallback(function(event)
            if not CONTEXTMAN:CheckContextSet(snm, "Main1") then
                resetTimeout()
                return
            end

            if event.type == "InputEventType_FirstPress" then
                if event.button == "MenuUp" or event.button == "Up" then
                    pressqueue[#pressqueue+1] = "Up"
                elseif event.button == "MenuDown" or event.button == "Down" then
                    pressqueue[#pressqueue+1] = "Down"
                end

                resetTimeout()

                -- potentially have the combo we want
                if #pressqueue >= 2 and pressqueue[#pressqueue-1] == pressqueue[#pressqueue] then
                    if pressqueue[#pressqueue] == "Up" then
                        currentindex = clamp(currentindex - 1, 1, #thesteps)
                        local newsteps = thesteps[currentindex]
                        self:GetChild("Cursor"):playcommand("ChangeSteps", {steps = newsteps})
                        pressqueue = {}
                    elseif pressqueue[#pressqueue] == "Down" then
                        currentindex = clamp(currentindex + 1, 1, #thesteps)
                        local newsteps = thesteps[currentindex]
                        self:GetChild("Cursor"):playcommand("ChangeSteps", {steps = newsteps})
                        pressqueue = {}
                    end
                end
            end
        end)
    end,
    SetCommand = function(self, params)
        if params.song then
            thesteps = WHEELDATA:GetChartsMatchingFilter(params.song)
            self:visible(true)
        else
            thesteps = {}
            self:visible(false)
        end
        setMaxWidthForSongInfo()
    end
}

local function stepsRows(i)
    local steps = nil
    local index = i

    local o = Def.ActorFrame {
        Name = "StepsFrame",
        InitCommand = function(self)
            -- to place indices 1-numshown left to right from the right to the left place them in reverse order
            self:x(-actuals.DiffItemWidth * (numshown - i) - actuals.DiffFrameSpacing * (numshown - i))
            self:visible(false)
        end,
        ColorConfigUpdatedMessageCommand = function(self)
            self:playcommand("UpdateStepsRows")
        end,
        UpdateStepsRowsCommand = function(self)
            -- to get them to align right
            index = pushIndexByBound(i)
            steps = thesteps[index + displayindexoffset]
            if steps then
                self:playcommand("SetStepsRows")
                self:visible(true)
            else
                self:visible(false)
            end
        end,

        UIElements.QuadButton(1) .. {
            Name = "BG",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:zoomto(actuals.DiffItemWidth, actuals.DiffItemHeight)
            end,
            SetStepsRowsCommand = function(self)
                local diff = steps:GetDifficulty()
                self:diffuse(colorByDifficulty(diff))
                self:diffusealpha(1)
            end,
            MouseDownCommand = function(self, params)
                if steps and params.event == "DeviceButton_left mouse button" then
                    -- tree:
                    -- StepsDisplayFile, StepsRows, StepsFrame, self
                    self:GetParent():GetParent():GetParent():GetChild("Cursor"):playcommand("ChangeSteps", {steps = steps})
                end
            end
        },
        Def.Quad {
            Name = "Lip",
            InitCommand = function(self)
                self:halign(1):valign(0)
                self:y(actuals.DiffItemHeight / 2)
                self:zoomto(actuals.DiffItemWidth, actuals.DiffItemHeight / 2)
            end,
            SetStepsRowsCommand = function(self)
                self:visible(true)
                self:diffuse(COLORS:getMainColor("SecondaryBackground"))
                self:diffusealpha(0.2)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "StepsType",
            InitCommand = function(self)
                self:xy(-actuals.DiffItemWidth / 2, actuals.DiffItemHeight / 4)
                self:zoom(textSize)
                self:maxwidth(actuals.DiffItemWidth / textSize - textzoomFudge)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
            end,
            SetStepsRowsCommand = function(self)
                local st = THEME:GetString("StepsDisplay StepsType", ToEnumShortString(steps:GetStepsType()))
                self:settext(st)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "NameAndMeter",
            InitCommand = function(self)
                self:xy(-actuals.DiffItemWidth / 2, actuals.DiffItemHeight / 4 * 3)
                self:maxwidth(actuals.DiffItemWidth / textSize - textzoomFudge)
                self:zoom(textSize)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
            end,
            SetStepsRowsCommand = function(self)
                local meter = steps:GetMeter()
                local diff = getShortDifficulty(steps:GetDifficulty())
                self:settextf("%s %s", diff, meter)
            end
        }

    }

    return o
end

local sdr = Def.ActorFrame {Name = "StepsRows"}

for i = 1, numshown do
    sdr[#sdr + 1] = stepsRows(i)
end
t[#t + 1] = sdr

local center = math.ceil(numshown / 2)

t[#t + 1] = Def.Sprite {
    Texture = THEME:GetPathG("", "stepsdisplayGlow"),
    Name = "Cursor",
    InitCommand = function(self)
        self:halign(1):valign(0)
        self:y(-actuals.DiffItemGlowVerticalSpan / 2)
        self:zoomto(actuals.DiffItemWidth + actuals.DiffItemGlowHorizontalSpan, actuals.DiffItemHeight + actuals.DiffItemGlowVerticalSpan)
        self:diffusealpha(1)
    end,
    ChangeStepsCommand = function(self, params)
        -- just to make sure nothing funny happens make sure that we cant get into an impossible chart
        if GAMESTATE:GetCurrentSong() == nil then return end

        -- actually do the work to set all game variables to make sure this diff plays if you press enter
        GAMESTATE:SetPreferredDifficulty(PLAYER_1, params.steps:GetDifficulty())
        GAMESTATE:SetCurrentSteps(PLAYER_1, params.steps)
        self:playcommand("Set", params)
        MESSAGEMAN:Broadcast("ChangedSteps", params)
    end,
    SetCommand = function(self, params)
        for i, chart in ipairs(thesteps) do
            if chart == params.steps then
                currentindex = i
                break
            end
        end

        -- probably just changed songs and need to reset the offset completely (hack kind of)
        if #thesteps <= numshown and displayindexoffset > #thesteps then
            displayindexoffset = 0
        end

        local cursorindex = currentindex
        if cursorindex <= center then
            displayindexoffset = clamp(displayindexoffset - 1, 0, #thesteps)
        elseif #thesteps - displayindexoffset > numshown then
            displayindexoffset = currentindex - center
            cursorindex = center
        else
            cursorindex = currentindex - displayindexoffset
        end

        if #thesteps > numshown and #thesteps - displayindexoffset < numshown then
            displayindexoffset = #thesteps - numshown
        end

        -- we have to offset the cursor to take into account the right alignment for lower numbers of diffs
        if #thesteps < numshown then
            local toOffset = pushIndexByBound(currentindex)
            cursorindex = numshown - #thesteps + cursorindex
        end

        if cursorindex < center and #thesteps > numshown then
            local newoff = displayindexoffset - 1
            if newoff >= 0 then
                displayindexoffset = math.max(displayindexoffset - 1, 0)
                cursorindex = cursorindex + 1
            end
        end

        -- find the left edge of the desired item, consider item width and gap width
        -- then offset by half the glow span (which is doubled for sizing)
        if thesteps[currentindex] then

            self:diffusealpha(1)
            -- positions relative to the right of the rightmost item
            -- rightmost index is numshown, move in reverse order
            self:x(-actuals.DiffItemWidth * (numshown - cursorindex) + -actuals.DiffFrameSpacing * (numshown - cursorindex) + actuals.DiffItemGlowHorizontalSpan / 2)
        else
            self:diffusealpha(0)
        end
        self:GetParent():GetChild("StepsRows"):queuecommand("UpdateStepsRows")
    end
}

return t
