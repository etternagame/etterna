-- the cover for the notefield
-- responsible for basically being the bms/iidx lane cover
-- and whatever else you want

-- dont load outside of gameplay
if Var("LoadingScreen") ~= nil and Var("LoadingScreen"):find("Gameplay") == nil then
    return Def.Actor {}
end

local laneColor = COLORS:getGameplayColor("LaneCover")
local bpmColor = COLORS:getGameplayColor("LaneCoverBPM")
local heightColor = COLORS:getGameplayColor("LaneCoverHeight")

local cols = GAMESTATE:GetCurrentStyle():ColumnsPerPlayer()
local evencols = cols - cols%2

-- load from prefs later
local nfspace = MovableValues.NoteFieldSpacing and MovableValues.NoteFieldSpacing or 0
local width = 64 * cols * MovableValues.NoteFieldWidth + nfspace * (evencols)
local padding = 20

local prefsP1 = playerConfig:get_data().LaneCover
local isReverseP1 = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()
if prefsP1 == 2 then -- it's a Hidden LaneCover
    isReverseP1 = not isReverseP1
end

local heightP1 = MovableValues.CoverHeight

if prefsP1 == 0 then
    return Def.Actor {Name = "Cover"}
end


local function getPlayerBPM()
    local songPosition = GAMESTATE:GetPlayerState():GetSongPosition()
    local ts = SCREENMAN:GetTopScreen()
    local bpm = 0
    if ts:GetScreenType() == "ScreenType_Gameplay" then
        bpm = ts:GetTrueBPS() * 60
    end
    return bpm
end

local function getMaxDisplayBPM()
    local steps = GAMESTATE:GetCurrentSteps()
    if steps:GetDisplayBPMType() ~= "DisplayBPM_Random" then
        return steps:GetDisplayBpms(false)[2]
    else
        return steps:GetTimingData():GetActualBPM()[2]
    end
end

local function getSpeed()
    local po = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
    if po:XMod() ~= nil then
        return po:XMod() * getPlayerBPM()
    elseif po:CMod() ~= nil then
        return po:CMod()
    elseif po:MMod() ~= nil then
        return po:MMod() * (getPlayerBPM() / getMaxDisplayBPM())
    else
        return getPlayerBPM()
    end
end

local yoffsetreverse = THEME:GetMetric("Player", "ReceptorArrowsYReverse")
local yoffsetstandard = THEME:GetMetric("Player", "ReceptorArrowsYStandard")

local function getNoteFieldHeight()
    local usingreverse = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()
    if usingreverse then
        return (SCREEN_CENTER_Y + yoffsetreverse / getNoteFieldScale())
    else
        return (SCREEN_CENTER_Y - yoffsetstandard / getNoteFieldScale())
    end
end

local function getScrollSpeed(LaneCoverHeight)
    local questionableNumber = 22 -- 22 is the "edge of screen" position for some reason???
    local height = getNoteFieldHeight()
    local speed = getSpeed()
    LaneCoverHeight = LaneCoverHeight - questionableNumber

    if LaneCoverHeight < height then
        return speed * (height / (height - LaneCoverHeight))
    else
        return 0
    end
end

local t = Def.ActorFrame {
    Name = "Cover",
    InitCommand = function(self)
        registerActorToCustomizeGameplayUI({
            actor = self,
            zoomInc = {5,1},
        }, 5)
        self:playcommand("SetUpMovableValues")
    end,
    SetUpMovableValuesMessageCommand = function(self)
        local wb4 = width
        local hb4 = heightP1
        width = 64 * cols * MovableValues.NoteFieldWidth + MovableValues.NoteFieldSpacing * (evencols)
        heightP1 = MovableValues.CoverHeight

        if width ~= wb4 or heightP1 ~= hb4 then
            local whitetext = self:GetChild("CoverTextP1White")
            local greentext = self:GetChild("CoverTextP1Green")
            whitetext:settext(math.floor(heightP1))
            if prefsP1 == 1 then -- don't update greennumber for hidden lanecovers
                greentext:settext(math.floor(getScrollSpeed(heightP1)))
            end

            if isReverseP1 then
                whitetext:y(heightP1 - 5 - getNoteFieldHeight()/2)
                greentext:y(heightP1 - 5 - getNoteFieldHeight()/2)
            else
                whitetext:y(getNoteFieldHeight()/2 - heightP1 + 5)
                greentext:y(getNoteFieldHeight()/2 - heightP1 + 5)
            end

            whitetext:finishtweening()
            whitetext:diffusealpha(1)
            whitetext:sleep(0.25)
            whitetext:smooth(0.75)
            whitetext:diffusealpha(0)

            greentext:finishtweening()
            greentext:diffusealpha(1)
            greentext:sleep(0.25)
            greentext:smooth(0.75)
            greentext:diffusealpha(0)

            whitetext:x(-(width / 8))
            greentext:x((width / 8))
        end

        self:playcommand("SetMovableWidths")
    end,
}

t[#t + 1] = Def.Quad {
    Name = "CoverP1",
    InitCommand = function(self)
        if isReverseP1 then
            self:y(-getNoteFieldHeight()/2)
            self:valign(0)
        else
            self:y(getNoteFieldHeight()/2)
            self:valign(1)
        end
        self:playcommand("SetMovableWidths")
        self:diffuse(laneColor)
        self:diffusealpha(1)
    end,
    SetMovableWidthsCommand = function(self)
        self:x(cols % 2 == 0 and -(MovableValues.NoteFieldSpacing and MovableValues.NoteFieldSpacing or 0) / 2 or 0)
        self:zoomto((width + padding), heightP1)
    end,
}

-- harming your fps in the name of making things look not bad if you have a bad setup
t[#t+1] = Def.Quad {
    Name = "CoverYouNeverSeeUnlessYouMoveTheNotefield",
    InitCommand = function(self)
        if isReverseP1 then
            self:y(-getNoteFieldHeight()/2)
            self:valign(1)
        else
            self:y(getNoteFieldHeight()/2)
            self:valign(0)
        end
        self:playcommand("SetMovableWidths")
        self:diffuse(laneColor)
        self:diffusealpha(1)
    end,
    SetMovableWidthsCommand = function(self)
        self:x(cols % 2 == 0 and -(MovableValues.NoteFieldSpacing and MovableValues.NoteFieldSpacing or 0) / 2 or 0)
        self:zoomto((width + padding), SCREEN_HEIGHT)
        if allowedCustomization then
            self:zoomy(0)
        end
    end,
}

t[#t + 1] = LoadFont("Common Normal") .. {
    Name = "CoverTextP1White",
    InitCommand = function(self)
        self:valign(1)
        self:playcommand("SetMovableWidths")
        self:zoom(0.5)
        self:diffuse(heightColor)
        self:diffusealpha(1)
        self:settext(0)
    end,
    BeginCommand = function(self)
        self:settext(0)
        if isReverseP1 then
            self:y(heightP1 - 5 - getNoteFieldHeight()/2)
            self:valign(1)
        else
            self:y(getNoteFieldHeight()/2 - heightP1 + 5)
            self:valign(0)
        end
        self:finishtweening()
        self:diffusealpha(1)
        self:sleep(0.25)
        self:smooth(0.75)
        self:diffusealpha(0)
    end,
    SetMovableWidthsCommand = function(self)
        self:x(-(width / 8))
    end,
}
t[#t + 1] = LoadFont("Common Normal") .. {
    Name = "CoverTextP1Green",
    InitCommand = function(self)
        self:valign(1)
        self:playcommand("SetMovableWidths")
        self:zoom(0.5)
        self:diffuse(bpmColor)
        self:diffusealpha(1)
        self:settext(0)
    end,
    SetMovableWidthsCommand = function(self)
        self:x((width / 8))
    end,
    BeginCommand = function(self)
        self:settext(math.floor(getSpeed(PLAYER_1)))
        if isReverseP1 then
            self:y(heightP1 - 5)
            self:valign(1)
        else
            self:y(SCREEN_BOTTOM - heightP1 + 5)
            self:valign(0)
        end
        self:finishtweening()
        self:diffusealpha(1)
        self:sleep(0.25)
        self:smooth(0.75)
        self:diffusealpha(0)
    end
}

return t
