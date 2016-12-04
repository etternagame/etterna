local update = {
	PLAYER_1 = false,
	PLAYER_2 = false,
}

--Setting to true will set the avatar as dirty, 
--in which the screen with the avatar that is checking every frame for updates will detect the change
--and re-load the avatar if the value from getAvatarUpdateStatus() returned true
function setAvatarUpdateStatus(pn,status)
	update[pn] = status
end;

--Returns the current avatar status
function getAvatarUpdateStatus(pn)
	return update[pn]
end;

--use global prefs instead of playerprefs as the playerprefs can't be grabbed when the profileslots aren't loaded.
local function addProfileFromGUID(GUID)
	if not tableContains(avatarConfig:get_data().avatar,GUID) then
		avatarConfig:get_data().avatar[GUID] = avatarConfig:get_data().avatar.default
		avatarConfig:set_dirty()
		avatarConfig:save()
	end;
end;

-- returns the image path relative to the theme folder for the specified player.
function getAvatarPath(pn)
	
	local fileName = avatarConfig:get_data().avatar.default

	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()

	fileName = avatarConfig:get_data().avatar[GUID]
	if fileName == nil then
		fileName = avatarConfig:get_data().avatar.default
		addProfileFromGUID(GUID)
	end;


	if FILEMAN:DoesFileExist("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/"..fileName) then
		return "Graphics/Player avatar/"..fileName
	else
		return "Graphics/Player avatar/_fallback.png"
	end;
end;

-- returns the image path relative to the theme folder from the profileID.
-- getAvatarPath should be used in the general case. this is really only needed for the profile select screen
-- where the profile isn't loaded into a player slot yet.
function getAvatarPathFromProfileID(id)
	local fileName = avatarConfig:get_data().avatar.default
	if id == nil then
		return fileName
	end

	local profile = PROFILEMAN:GetLocalProfile(id)
	local GUID = profile:GetGUID()

	fileName = avatarConfig:get_data().avatar[GUID]
	if fileName == nil then
		fileName = avatarConfig:get_data().avatar.default
		addProfileFromGUID(GUID)
	end;

	if FILEMAN:DoesFileExist("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/"..fileName) then
		return "Graphics/Player avatar/"..fileName
	else
		return "Graphics/Player avatar/_fallback.png"
	end;
end;

-- Creates an actor with the avatar image.
-- Unused, it's more for testing.
function getAvatar(pn)
	local fileName = avatarConfig:get_data().avatar.default

	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()

	fileName = avatarConfig:get_data().avatar[GUID]
	if fileName == nil then
		fileName = avatarConfig:get_data().avatar.default
		addProfileFromGUID(GUID)
	end;

	if FILEMAN:DoesFileExist("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/"..fileName) then
		t = LoadActor("../Graphics/Player avatar/"..fileName)..{
			Name="Avatar";
			InitCommand=cmd(visible,true;zoomto,50,50;halign,0;valign,0;);
		};
	else
		t = LoadActor("../Graphics/Player avatar/generic")..{
			Name="Avatar";
			InitCommand=cmd(visible,true;zoomto,50,50;halign,0;valign,0;);
		};
	end

	return t
end;

-- Old functions, new functions are now tied to kyzentun's prefs system and is here just for reference.
-- Returns a path to the avatar image (relative to the theme folder) for the specified player.
--[[
--shamelessly copied from customspeedmod
local function ReadAvatarFile(path)
	local file = RageFileUtil.CreateRageFile()
	if not file:Open(path, 1) then
		file:destroy()
		return nil
	end

	local contents = file:Read()
	file:Close()
	file:destroy()

	return contents
end

function getAvatarPath(pn)
	if pn == nil then
		return "Graphics/Player avatar/_fallback.png"
	end
	local profile
	local profilePath = ""
	local fileName = "_fallback.png"
	if GAMESTATE:IsPlayerEnabled(pn) then
		profile = GetPlayerOrMachineProfile(pn)
		if pn == PLAYER_1 then
			profilePath = PROFILEMAN:GetProfileDir('ProfileSlot_Player1')
			fileName = ReadAvatarFile(profilePath.."/avatar.txt")
		elseif pn == PLAYER_2 then
			profilePath = PROFILEMAN:GetProfileDir('ProfileSlot_Player2')
			fileName = ReadAvatarFile(profilePath.."/avatar.txt")
		else
			fileName = nil
		end;	
		if fileName == nil then
			fileName = "_fallback.png"
		end
	end

	if FILEMAN:DoesFileExist("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/"..fileName) then
		return "Graphics/Player avatar/"..fileName
	else
		return "Graphics/Player avatar/_fallback.png"
	end;
end;

function getAvatarPathFromProfileID(id)
	if id == nil then
		return "Graphics/Player avatar/_fallback.png"
	end
	local profilePath = ""
	local fileName = "_fallback.png"

	profilePath = PROFILEMAN:LocalProfileIDToDir(id)
	fileName = ReadAvatarFile(profilePath.."/avatar.txt")

	if fileName == nil then
		fileName = "_fallback.png"
	end

	if FILEMAN:DoesFileExist("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/"..fileName) then
		return "Graphics/Player avatar/"..fileName
	else
		return "Graphics/Player avatar/_fallback.png"
	end;
end;

-- Creates an actor with the avatar image.
function getAvatar(pn)
	local profile
	local profilePath = ""
	local fileName = "_fallback.png"
	if GAMESTATE:IsPlayerEnabled(pn) then
		profile = GetPlayerOrMachineProfile(pn)
		profilePath = PROFILEMAN:GetProfileDir('ProfileSlot_Player1')
		fileName = ReadAvatarFile(profilePath.."/avatar.txt")
		if fileName == nil then
			fileName = "_fallback.png"
		end
	end

	if FILEMAN:DoesFileExist("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/"..fileName) then
		t = LoadActor("../Graphics/Player avatar/"..fileName)..{
			Name="Avatar";
			InitCommand=cmd(visible,true;zoomto,50,50;halign,0;valign,0;);
		};
	else
		t = LoadActor("../Graphics/Player avatar/generic")..{
			Name="Avatar";
			InitCommand=cmd(visible,true;zoomto,50,50;halign,0;valign,0;);
		};
	end

	return t
end;
--]]
