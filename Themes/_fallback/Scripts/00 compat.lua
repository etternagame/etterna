-- this file is for some lua backwards compatibility purposes
-- add to this as C++ Lua bindings become refactored or renamed as needed


-- ArchHooks
-- used to be referred to as HOOKS:f(etc)
-- now Arch.f(etc)
-- DO NOT SHADOW THIS NAME
HOOKS = {}
-- old names for compat
Arch["AppHasFocus"] = Arch.isGameFocused
Arch["GetArchName"] = Arch.getSystem
Arch["GetClipboard"] = Arch.getClipboard
Arch["ShowCursor"] = Arch.setCursorVisible
for k,v in pairs(Arch) do
    HOOKS[k] = function(_, ...) return v(...) end
end
