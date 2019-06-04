local update = {
	PLAYER_1 = false
}

--Setting to true will set the avatar as dirty,
--in which the screen with the avatar that is checking every frame for updates will detect the change
--and re-load the avatar if the value from getAvatarUpdateStatus() returned true
function setAvatarUpdateStatus(pn, status)
	update[PLAYER_1] = status
end

--Returns the current avatar status
function getAvatarUpdateStatus()
	return update[PLAYER_1]
end

--use global prefs instead of playerprefs as the playerprefs can't be grabbed when the profileslots aren't loaded.
local function addProfileAssetFromGUID(GUID, asset)
	if not asset then
		asset = "avatar"
	end
	if not tableContains(assetsConfig:get_data().avatar, GUID) then
		assetsConfig:get_data()[asset][GUID] = getDefaultAssetByType(asset)
		assetsConfig:set_dirty()
		assetsConfig:save()
	end
end

-- returns the image path relative to the theme folder for the specified player.
function getAssetPath(asset)
	local pn = PLAYER_1
	local fileName = getDefaultAssetByType(asset)

	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()

	fileName = assetsConfig:get_data()[asset][GUID]
	if fileName == nil then
		fileName = getDefaultAssetByType(asset)
		addProfileAssetFromGUID(GUID, asset)
	end

	if FILEMAN:DoesFileExist(fileName) then
		return fileName
	else
		return getDefaultAssetByType(asset)
	end
end
function getAvatarPath()
	return getAssetPath("avatar")
end

-- returns the image path relative to the theme folder from the profileID.
-- getAvatarPath should be used in the general case. this is really only needed for the profile select screen
-- where the profile isn't loaded into a player slot yet.
function getAssetPathFromProfileID(asset, profileID)
	if not asset then
		asset = "avatar"
	end
	local fileName = getDefaultAssetByType(asset)
	if profileID == nil then
		return fileName
	end

	local profile = PROFILEMAN:GetLocalProfile(profileID)
	local GUID = profile:GetGUID()

	fileName = assetsConfig:get_data()[asset][GUID]
	if fileName == nil then
		fileName = getDefaultAssetByType(asset)
		addProfileAssetFromGUID(GUID, asset)
	end

	if FILEMAN:DoesFileExist(fileName) then
		return fileName
	else
		return getDefaultAssetByType(asset)
	end
end

function getAvatarPathFromProfileID(id)
	return getAssetPathFromProfileID(id)
end

-- Creates an actor with the asset image.
-- Unused, it's more for testing.
function getAsset(asset)
	local pn = PLAYER_1
	local fileName = getDefaultAssetByType(asset)

	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()

	fileName = assetsConfig:get_data()[asset][GUID]
	if fileName == nil then
		fileName = getDefaultAssetByType(asset)
		addProfileFromGUID(GUID)
	end

	local file
	if FILEMAN:DoesFileExist(fileName) then
		file = fileName
	else
		file = getDefaultAssetByType(asset)
	end
	t =
		LoadActor(file) ..
		{
			Name = asset,
			InitCommand = function(self)
				self:visible(true):zoomto(50, 50):halign(0):valign(0)
			end
		}

	return t
end

function getAvatar()
	return getAsset("avatar")
end
