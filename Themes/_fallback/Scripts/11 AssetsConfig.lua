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

function getDefaultAssetByType(type)
	return defaultConfig[type].default
end

function setAssetsByType(type, profile_ID, path)
	assetsConfig:get_data("ProfileSlot_Player1")[type][profile_ID] = path
	assetsConfig:set_dirty("ProfileSlot_Player1")
	assetsConfig:save("ProfileSlot_Player1")
end

function getAssetByType(type, profile_ID)
	local section = assetsConfig:get_data("ProfileSlot_Player1")[type]
	if section == nil then return getDefaultAssetByType(type) end
	local path = section[profile_ID]
	if path == nil then return getDefaultAssetByType(type) end
	return path
end