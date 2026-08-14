
-- these lua based music resolvers have been known to crash really easily
-- if an error is thrown that probably causes a crash most of the time

if themeConfig and playSongSelectBGM ~= nil and playSongSelectBGM() then
    return THEME:GetPathS("", "music/quiver")
else
    return THEME:GetPathS("", "_silent")
end