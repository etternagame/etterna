local defaultConfig = {
	playerTags = {}
}

tags = create_setting("tags", "tags.lua", defaultConfig, 0)
tags:load()
