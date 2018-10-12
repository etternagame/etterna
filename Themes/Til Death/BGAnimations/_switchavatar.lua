local function input(event)
	if event.DeviceInput.button == "DeviceButton_left mouse button" then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("AvMouseLeftClick")
		end
	end
	if event.type ~= "InputEventType_Release" then
		if event.GameButton == "MenuLeft" then
			MESSAGEMAN:Broadcast("AvLeft")
		elseif event.GameButton == "MenuRight" then
			MESSAGEMAN:Broadcast("AvRight")
		elseif event.GameButton == "Back" then
			MESSAGEMAN:Broadcast("AvCancel")
		elseif event.GameButton == "Start" then
			MESSAGEMAN:Broadcast("AvExit")
		end
	end
	return false
end

--Parameters.
local imgTypes = {".jpg", ".png", ".gif", ".jpeg"}
local rawList = FILEMAN:GetDirListing(assetFolders.avatar)
local avatars = filterFileList(rawList, imgTypes)

local maxItems = 7
--math.min(7,#avatars)
local itemHeight = 30
local itemWidth = 30
local border = 5
local frameX = 0
local frameY = SCREEN_HEIGHT - 55
local height = itemHeight + (border * 2)
local width = maxItems * (itemWidth + border) + border

--search for avatar currently being used. if none are found, revert to _fallback.png which is assumed to be on index 1.
local function getInitAvatarIndex(pn)
	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()
	local avatar = avatarConfig:get_data().avatar[GUID]
	for i = 1, #avatars do
		if avatar == avatars[i] then
			return i
		end
	end

	return 1
end

--place cursor on center unless it's on the edge.
local function getInitCursorIndex(pn)
	local avatarIndex = getInitAvatarIndex(pn)

	if avatarIndex < math.ceil(maxItems / 2) then
		return avatarIndex
	elseif avatarIndex > #avatars - math.ceil(maxItems / 2) then
		return maxItems - (#avatars - avatarIndex)
	end
	return math.ceil(maxItems / 2)
end

local data = {
	PlayerNumber_P1 = {
		cursorIndex = getInitCursorIndex(PLAYER_1),
		avatarIndex = getInitAvatarIndex(PLAYER_1)
	}
}

local t =
	Def.ActorFrame {
	Name = "AvatarSwitch",
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end
}

--Shifts an actor by "1 index"
local function shift(actor, amount)
	actor:finishtweening()
	actor:smooth(0.1)
	actor:addx((itemWidth + border) * amount)
end

--Grabs the currently selected avatar.
local function getSelectedAvatar(pn)
	return avatars[data[pn]["avatarIndex"]]
end

--Save preferences and sends a systemmessage at the end.
local function saveAvatar(pn)
	local avatar = getSelectedAvatar(pn)
	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()
	local profileName = profile:GetDisplayName()
	avatarConfig:get_data().avatar[GUID] = avatar
	avatarConfig:set_dirty()
	avatarConfig:save()
	SCREENMAN:SystemMessage(string.format("%s's avatar set to: '%s'", profileName, avatar))
end

-- The main function that contains errything
local function avatarSwitch(pn)
	local t =
		Def.ActorFrame {
		Name = "AvatarSwitch" .. pn,
		BeginCommand = function(self)
			self:x(-width)
			self:sleep(0.3)
			self:smooth(0.2)
			self:x(0)
		end,
		AvExitMessageCommand = function(self)
			self:smooth(0.2)
			self:x(-width)
		end,
		AvCancelMessageCommand = function(self)
			self:smooth(0.2)
			self:x(-width)
		end
	}

	t[#t + 1] =
		Def.ActorFrame {
		AvLeftMessageCommand = function(self)
			--grab table/cursor and shift them by 1 to left/right everytime someone presses code for avatarleft/right
			local avatarAttributes =
				SCREENMAN:GetTopScreen():GetChildren().Overlay:GetChildren().AvatarSwitch:GetChildren()["AvatarSwitch" .. pn]:GetChildren(

			)
			local table = avatarAttributes.AvatarTable
			local cursor = avatarAttributes.AvatarCursor
			if data[pn]["avatarIndex"] > 1 and data[pn]["cursorIndex"] > 1 then
				shift(cursor, -1)
				data[pn]["avatarIndex"] = data[pn]["avatarIndex"] - 1
				data[pn]["cursorIndex"] = data[pn]["cursorIndex"] - 1
			elseif data[pn]["avatarIndex"] > 1 and data[pn]["cursorIndex"] == 1 then
				shift(table, 1)
				data[pn]["avatarIndex"] = data[pn]["avatarIndex"] - 1
			end
		end,
		AvRightMessageCommand = function(self)
			--grab table/cursor and shift them by 1 to left/right everytime someone presses code for avatarleft/right
			local avatarAttributes =
				SCREENMAN:GetTopScreen():GetChildren().Overlay:GetChildren().AvatarSwitch:GetChildren()["AvatarSwitch" .. pn]:GetChildren(

			)
			local table = avatarAttributes.AvatarTable
			local cursor = avatarAttributes.AvatarCursor
			if data[pn]["avatarIndex"] < #avatars and data[pn]["cursorIndex"] < maxItems then
				shift(cursor, 1)
				data[pn]["avatarIndex"] = data[pn]["avatarIndex"] + 1
				data[pn]["cursorIndex"] = data[pn]["cursorIndex"] + 1
			elseif data[pn]["avatarIndex"] < #avatars and data[pn]["cursorIndex"] == maxItems then
				shift(table, -1)
				data[pn]["avatarIndex"] = data[pn]["avatarIndex"] + 1
			end
		end,
		--rq out of the screen if just canceling.
		AvCancelMessageCommand = function(self)
			SCREENMAN:GetTopScreen():Cancel()
		end,
		AvExitMessageCommand = function(self)
			saveAvatar(pn)
			setAvatarUpdateStatus(pn, true)
			SCREENMAN:GetTopScreen():Cancel()
		end
	}

	--Background Quad
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY):zoomto(width, height):halign(0):valign(1):diffuse(color("#00000066"))
		end
	}

	--MASKING SCKS
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(width, 0):zoomto(SCREEN_WIDTH - width, SCREEN_HEIGHT):halign(0):valign(0):zwrite(true):clearzbuffer(true):blend(
				"BlendMode_NoEffect"
			)
		end
	}

	--Cursor
	t[#t + 1] =
		Def.Quad {
		Name = "AvatarCursor",
		InitCommand = function(self)
			self:xy(frameX - 2 + border, frameY + 2 - border):zoomto(itemHeight + 4, itemWidth + 4):halign(0):valign(1):diffuse(
				color("#FFFFFF")
			)
		end,
		BeginCommand = function(self)
			shift(self, (data[pn]["cursorIndex"] - 1))
		end
	}

	--List of avatars
	local avatarTable =
		Def.ActorFrame {
		Name = "AvatarTable",
		BeginCommand = function(self)
			shift(self, -(data[pn]["avatarIndex"] - 1))
			shift(self, (data[pn]["cursorIndex"] - 1))
		end
	}
	t[#t + 1] = avatarTable
	for k, v in pairs(avatars) do
		avatarTable[#avatarTable + 1] =
			Def.Sprite {
			Name = #avatarTable,
			InitCommand = function(self)
				self:visible(true):halign(0):valign(1):xy(frameX + border + ((border + itemWidth) * (k - 1)), frameY - border):ztest(
					true
				)
			end,
			BeginCommand = function(self)
				self:queuecommand("ModifyAvatar")
			end,
			ModifyAvatarCommand = function(self)
				self:finishtweening()
				self:LoadBackground(assetFolders.avatar .. v)
				self:zoomto(itemWidth, itemHeight)
			end,
			AvMouseLeftClickMessageCommand = function(self) -- theoretically this should move the scroller unless to the target, unless the target is already selected in which case this should executre, but, that's work -mina
				if isOver(self) then
					data[pn]["avatarIndex"] = tonumber(self:GetName()) + 1
					saveAvatar(pn)
					setAvatarUpdateStatus(pn, true)
					SCREENMAN:GetTopScreen():Cancel()
				end
			end
		}
	end

	--Text
	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY - height):halign(0):valign(1):zoom(0.35)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self, params)
				local profileName = GetPlayerOrMachineProfile(PLAYER_1):GetDisplayName()
				self:settextf("%s's avatar: %s", profileName, avatars[data[pn]["avatarIndex"]])
			end,
			AvExitMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
	return t
end

if GAMESTATE:IsHumanPlayer(PLAYER_1) then
	t[#t + 1] = avatarSwitch(PLAYER_1)
end

return t
