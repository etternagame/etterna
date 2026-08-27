-- these lua based music resolvers have been known to crash really easily
-- if an error is thrown that probably causes a crash most of the time

if themeConfig and playTitleMusic ~= nil and playTitleMusic() then
    return THEME:GetPathS("", "music/longarrowhead")
else
    return THEME:GetPathS("", "_silent")
end
