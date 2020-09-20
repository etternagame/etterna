return Def.ActorFrame {
    Name = "TextEntryOverlayFile",

    LoadFont("Common Normal") .. {
        Name = "Failure",
        InitCommand = function(self)
            self:visible(false)
        end
    }
}