local t = Def.ActorFrame {
    OnCommand = function(self)
        SCREENMAN:GetTopScreen():AddInputCallback(MPinput)
    end,
}

-- header
t[#t+1] = LoadActorWithParams("../playerInfoFrame/main.lua", {visualizer = themeConfig:get_data().global.ShowVisualizer, screen = "ScreenSelectMusic"})
updateDiscordStatusForMenus()
updateNowPlaying()

local scnm = Var ("LoadingScreen")
if scnm ~= nil and scnm:find("Net") ~= nil then
    t[#t+1] = LoadActor("multiuserlist")
end

t[#t+1] = LoadActor("../_mouse.lua")

return t