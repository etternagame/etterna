--- Backwards compatibility configuration
-- This file is for some Lua backwards compatibility purposes.
-- Add to this as C++ Lua bindings become refactored or renamed as needed.
-- Right now it is only used to keep the old HOOKS ArchHooks class alive.
-- @module 00_compat

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
