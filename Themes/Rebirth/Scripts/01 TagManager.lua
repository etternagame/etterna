local defaultConfig = {
    playerTags = {}
}

TAGMAN = create_setting("tags", "tags.lua", defaultConfig, 0)
TAGMAN:load()
