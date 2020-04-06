local defaultConfig = {
	avatar = {
		default = "Assets/Avatars/_fallback.png"
	},
	judgment = {
		default = "Assets/Judgments/default 1x6 (Doubleres).png"
	},
	toasty = {
		default = "Assets/Toasties/default"
	}
}

assetsFolder = "Assets/"
assetFolders = {
	avatar = assetsFolder .. "Avatars/",
	judgment = assetsFolder .. "Judgments/",
	toasty = assetsFolder .. "Toasties/"
}

assetsConfig = create_setting("assetsConfig", "assetsConfig.lua", defaultConfig, 1)
assetsConfig:load()
avatarConfig = assetsConfig

local function isImage(filename)
	local extensions = {".png", ".jpg", "jpeg"} -- lazy list
	local ext = string.sub(filename, #filename-3)
	for i=1, #extensions do
		if extensions[i] == ext then return filename end
	end
	return false
end

local function isAudio(filename)
	local extensions = {".wav", ".mp3", ".ogg", ".mp4"} -- lazy to check and put in names
	local ext = string.sub(filename, #filename-3)
	for i=1, #extensions do
		if extensions[i] == ext then return filename end
	end
	return false
end

function getToastyAssetPath(type)
	local path = getAssetPath("toasty")
	local files = FILEMAN:GetDirListing(path.."/")
	if type == "sound" then
		for i=1, #files do
			local status = isAudio(files[i])
			if status then
				return path .. "/" .. status
			end
		end
	end
	if type == "image" then
		for i=1, #files do
			local status = isImage(files[i])
			if status then
				return path .. "/" .. status
			end
		end
	end
	return path
end

function findAssetsForPath(path)
	local ext = string.sub(path, #path-3)
	if not isAudio(ext) and not isImage(ext) then
		return FILEMAN:GetDirListing(path.."/")
	else
		local out = {}
		out[1] = path
		return out
	end
end

function getDefaultAssetByType(type)
	local section = defaultConfig[type]
	if section == nil then return "" end
	return section.default
end

function setAssetsByType(type, profile_ID, path)
	assetsConfig:get_data()[type][profile_ID] = path
	assetsConfig:set_dirty()
	assetsConfig:save()
end

function getAssetByType(type, profile_ID)
	local section = assetsConfig:get_data()[type]
	if section == nil then return getDefaultAssetByType(type) end
	local path = section[profile_ID]
	if path == nil then return getDefaultAssetByType(type) end
	return path
end
