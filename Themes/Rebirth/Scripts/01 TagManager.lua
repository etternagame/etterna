--- Rebirth Tag Manager
-- @module Rebirth_01_TagManager
local defaultConfig = {
    playerTags = {}
}

TAGMAN = create_setting("tags", "tags.lua", defaultConfig, 0)
TAGMAN:load()
