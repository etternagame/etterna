-- this delegates the existence and further control and customization of all gameplay elements
-- decided to put this into its own folder for organization related reasons 
local customizationEnabled = false


local t = Def.ActorFrame {Name = "CustomGameplayElementLoader"}


if customizationEnabled then
    t[#t+1] = LoadActor("gameplaycustomization")
end

t[#t+1] = LoadActor("gameplayelements")

return t