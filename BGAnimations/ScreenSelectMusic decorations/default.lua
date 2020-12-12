local t = Def.ActorFrame {}

local rightFrameVisible = true

t[#t+1] = LoadActor("wheel")

t[#t+1] = Def.ActorFrame {
    Name = "RightFrame",
    GeneralTabSetMessageCommand = function(self, params)
        if params ~= nil and params.tab ~= nil then
            SCUFF.generaltab = params.tab
        end
        if not rightFrameVisible then
            CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Main1")
            self:finishtweening()
            self:smooth(0.1)
            self:x(0)
            rightFrameVisible = true
        end
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self)
        rightFrameVisible = false
        self:finishtweening()
        self:smooth(0.1)
        self:x(SCREEN_WIDTH)
    end,

    LoadActor("curSongBox"),
    LoadActor("generalBox"),
}

return t