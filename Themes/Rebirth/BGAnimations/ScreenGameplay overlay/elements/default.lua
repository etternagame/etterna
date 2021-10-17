-- this delegates the existence and further control and customization of all gameplay elements
-- decided to put this into its own folder for organization related reasons 
local customizationEnabled = playerConfig:get_data().CustomizeGameplay == true
local practiceEnabled = GAMESTATE:IsPracticeMode()
local replayEnabled = GAMESTATE:GetGameplayMode() == "GameplayMode_Replay"

if not replayEnabled and not customizationEnabled and not practiceEnabled then
	Arch.setCursorVisible(false)
end


local t = Def.ActorFrame {Name = "CustomGameplayElementLoader"}


t[#t+1] = LoadActor("_gameplayelements")

if practiceEnabled then
    t[#t+1] = LoadActor("_gameplaypractice")
end

if replayEnabled then
    t[#t+1] = LoadActor("_gameplayreplay")
end

if customizationEnabled then
    t[#t+1] = LoadActor("_gameplaycustomization")
end

if practiceEnabled or replayEnabled or customizationEnabled then
    t[#t+1] = LoadActor("../../_mouse.lua")
end

return t