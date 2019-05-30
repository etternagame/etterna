local defaultConfig = {
	avatar = {
		default = "_fallback.png"
	},
	judgement = {
		default = "default 1x6.png"
	},
	toasty = {
		default = "default"
	}
}

assetsFolder = "Assets/"
assetFolders = {
	avatar = assetsFolder .. "Avatars/",
	judgement = assetsFolder .. "Judgements/",
	toasty = assetsFolder .. "Toasties/"
}

assetsConfig = create_setting("assetsConfig", "assetsConfig.lua", defaultConfig, 0)
assetsConfig:load()
avatarConfig = assetsConfig
