local t = Def.ActorFrame {
    OnCommand = function(self)
        -- this just serves the purpose to make sure that the player can actually use input
        SCREENMAN:set_input_redirected(PLAYER_1, false)
    end,
}



return t