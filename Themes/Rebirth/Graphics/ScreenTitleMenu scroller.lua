local gc = Var("GameCommand")
local choiceTable = strsplit(THEME:GetMetric("ScreenTitleMenu", "ChoiceNames"), ",")
local transitionTable = strsplit(THEME:GetMetric("ScreenTitleMenu", "ChoicesExitScreen"), ",")
local screen

local choiceTextZoom = 0.6
local buttonVerticalFudge = 5

-- initial width of the selector
-- it may be bigger or smaller in reality due to item resizing done in ScreenTitleMenu underlay
local selectorWidth = 574 / 1920 * SCREEN_WIDTH

-- look through the choices defined in metrics.ini [ScreenTitleMenu] ChoiceNames
-- the Scrollers (choices) are named "ScrollChoice<GameCommandName>" so we just have to look for it that way
-- we offset it by 1 to index it from 0: the index being sent back to c++ needs to be 0 indexed
-- keep that in mind if reusing it somewhere else in a table
-- index of 1 returned by this function is the 2nd item in the choiceTable
local function findChoiceIndex(name)
    local ind = 0
    for i, nam in ipairs(choiceTable) do
        if "ScrollChoice"..nam == name then
            ind = i - 1
        end
    end
    return ind
end

-- we keep track of which screen choices will cause a transition to the next screen or something else
local function choiceWillTransitionOut(name)
    for i, nam in ipairs(transitionTable) do
        if nam == name then
            return true
        end
    end
    return false
end

return Def.ActorFrame {
    BeginCommand = function(self)
        screen = SCREENMAN:GetTopScreen()
    end,
    -- the name of this frame is determined by C++
    -- it will be the name of the choice
    -- ie: ScrollerChoice<GameCommandName>

    LoadFont("Common Large") ..    {
        Name = "ScrollerText",
        BeginCommand = function(self)
            self:halign(0)
            self:zoom(choiceTextZoom)
            self:settext(THEME:GetString(screen:GetName(), gc:GetText()))
            self:diffuse(COLORS:getTitleColor("PrimaryText"))
            self:diffusealpha(1)
        end
    },
    UIElements.QuadButton(1, 1) .. {
        Name = "Button",
        BeginCommand = function(self)
            self:halign(0)
            self:diffusealpha(0)
            local txt = self:GetParent():GetChild("ScrollerText")
            self:zoomto(selectorWidth, txt:GetZoomedHeight() + buttonVerticalFudge)
        end,
        MouseOverCommand = function(self)
            -- if not focused on the scroller, don't allow controlling it
            if not TITLE:GetFocus() then return end
            local ind = findChoiceIndex(self:GetParent():GetName())
            screen:SetSelectionIndex(ind)
        end,
        MouseDownCommand = function(self)
            -- if not focused on the scroller, don't allow controlling it
            if not TITLE:GetFocus() then return end

            -- this should make clicking work the same as pressing enter
            screen:PlaySelectSound()
            screen:playcommand("MadeChoiceP1")
            screen:playcommand("Choose")
            MESSAGEMAN:Broadcast("MenuStartP1")

            -- add 1 to this index to convert c++ index to lua table index
            local ind = findChoiceIndex(self:GetParent():GetName()) + 1
            local choice = choiceTable[ind]

            -- have to join players if playing the game or whatever
            -- this might be moved to force join on game start for profile selection in main menu
            GAMESTATE:JoinPlayer(PLAYER_1)
            if choiceWillTransitionOut(choice) then
                screen:PostScreenMessage("SM_BeginFadingOut", 0)
            else
                GAMESTATE:ApplyGameCommand(THEME:GetMetric("ScreenTitleMenu", "Choice"..choice))
            end
        end
    }
}
