local defaultConfig = {
	avatar = {
		default = "_fallback.png",
	},
}

avatarConfig = create_setting("avatarConfig", "avatarConfig.lua", defaultConfig,0)
avatarConfig:load()