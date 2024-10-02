local t = Def.ActorFrame {
    OnCommand = function(self)
        SCREENMAN:GetTopScreen():AddInputCallback(MPinput)
    end,
}

t[#t+1] = LoadActor("../_mouse.lua")

-- header
t[#t+1] = LoadActorWithParams("../playerInfoFrame/main.lua", {visualizer = themeConfig:get_data().global.ShowVisualizer, screen = "ScreenSelectMusic"})
updateDiscordStatusForMenus()
updateNowPlaying()

return t