-- this is a kind of manager for the functionality on the title menu
-- initially, we are focused on the scroller options
-- we want to be able to handle the transition between the options and the profile choices
-- this is the simplest way to do it without remaking the entire screen in lua

TITLE = {
    scrollerFocused = true, -- focused on the main choices
    doingmulti = false,
    nextScreen = "ScreenTitleMenu",
    triggeredFadeOut = false,
}

function TITLE.GetFocus(self)
    return self.scrollerFocused
end

function TITLE.SetFocus(self, status)
    self.scrollerFocused = status
end

function TITLE.ChangeFocus(self)
    self.scrollerFocused = not self.scrollerFocused
    MESSAGEMAN:Broadcast("ToggledTitleFocus", {scrollerFocused = self.scrollerFocused})
end

function TITLE.HandleFinalGameStart(self)
    if self.triggeredFadeOut then return end
    self.triggeredFadeOut = true
    self.scrollerFocused = true
    GAMESTATE:JoinPlayer()
    GAMESTATE:LoadProfiles(false)

    local top = SCREENMAN:GetTopScreen()
    if TITLE.doingmulti then
        SCREENMAN:set_input_redirected(PLAYER_1, false)
        TITLE.nextScreen = Branch.MultiScreen()
    else
        TITLE.nextScreen = "ScreenSelectMusic"
    end
    top:PostScreenMessage("SM_BeginFadingOut", 0)
end

function TITLE.NextFromTitle()
    return TITLE.nextScreen
end

function TITLE.GameStartOnTheScroller(pn)
    TITLE.doingmulti = false
    TITLE:ChangeFocus()
    return true
end

function TITLE.MultiplayerOnTheScroller(pn)
    TITLE.doingmulti = true
    TITLE:ChangeFocus()
    return true
end