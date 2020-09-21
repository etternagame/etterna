-- this is a kind of manager for the functionality on the title menu
-- initially, we are focused on the scroller options
-- we want to be able to handle the transition between the options and the profile choices
-- this is the simplest way to do it without remaking the entire screen in lua

TITLE = {
    scrollerFocused = true, -- focused on the main choices
}

function TITLE.SetFocus(self, status)
    self.scrollerFocused = status
end

function TITLE.ChangeFocus(self)
    self.scrollerFocused = not self.scrollerFocused
    MESSAGEMAN:Broadcast("ToggledTitleFocus", {scrollerFocused = self.scrollerFocused})
end

function TITLE.HandleFinalGameStart(self)
    self.scrollerFocused = true
    GAMESTATE:LoadProfiles(false)

    SCREENMAN:SetNewScreen("ScreenSelectMusic")
end

function TITLE.GameStartOnTheScroller(pn)
    TITLE:ChangeFocus()
    return true
end

function TITLE.MultiplayerOnTheScroller(pn)
    TITLE:ChangeFocus()
    return true
end