local level = 0
local cursor = 0
local count = 0

local selected = getTableKeys()
local themeColor = colorConfig:get_data()[selected[1]][selected[2]]
local colorTable = {}
for i = 2, #themeColor do --First string is a "#", ignore.
	colorTable[i - 1] = themeColor:sub(i, i)
end

local translated_info = {
	Title = THEME:GetString("ScreenColorEdit", "Title"),
	Description = THEME:GetString("ScreenColorEdit", "Description")
}

local function scroller(index)
	count = count + 1
	local number = tonumber(colorTable[index], 16)
	local t = Def.ActorFrame {}

	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(SCREEN_CENTER_X - 60 + index * 20, SCREEN_CENTER_Y):zoom(0.8)
			end,
			OnCommand = function(self)
				self:settext(string.format("%01X", number or 0))
				if index == cursor then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			CodeMessageCommand = function(self, params)
				if params.Name == "ColorUp" then
					if index == cursor then
						number = (number + 1) % 16
						self:settext(string.format("%01X", number or 0))
						colorTable[index] = string.format("%01X", number or 0)
					end
				end
				if params.Name == "ColorDown" then
					if index == cursor then
						number = (number - 1) % 16
						self:settext(string.format("%01X", number or 0))
						colorTable[index] = string.format("%01X", number or 0)
					end
				end
				if params.Name == "ColorLeft" then
					if index == cursor then
						self:diffuse(color("#FFFFFF"))
					else
						self:diffuse(color("#666666"))
					end
				end
				if params.Name == "ColorRight" then
					if index == cursor then
						self:diffuse(color("#FFFFFF"))
					else
						self:diffuse(color("#666666"))
					end
				end
				if params.Name == "ColorStart" then
					if index == cursor then
						self:diffuse(color("#FFFFFF"))
					else
						self:diffuse(color("#666666"))
					end
				end
			end
		}

	return t
end

local t =
	Def.ActorFrame {
	CodeMessageCommand = function(self, params)
		if params.Name == "ColorCancel" then
			SCREENMAN:GetTopScreen():Cancel()
		end
		if params.Name == "ColorStart" then
			if cursor < 6 then
				cursor = ((cursor) % (count)) + 1
			else
				colorConfig:get_data()[selected[1]][selected[2]] = "#" .. table.concat(colorTable)
				colorConfig:set_dirty()
				colorConfig:save()
				MESSAGEMAN:Broadcast("RowChanged", {level = 1})
				THEME:ReloadMetrics()
				SCREENMAN:GetTopScreen():Cancel()
			end
		end
		if params.Name == "ColorRight" then
			cursor = ((cursor) % (count)) + 1
		end
		if params.Name == "ColorLeft" then
			cursor = ((cursor - 2) % (count)) + 1
		end
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(0, 0):halign(0):valign(0):zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(color("#000000EE"))
	end
}

t[#t + 1] = LoadActor("_frame")
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("highlight")):settext(translated_info["Title"])
		end
	}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y + 40):zoomto(200, 30)
	end,
	OnCommand = function(self)
		self:diffuse(color(themeColor))
	end,
	CodeMessageCommand = function(self, params)
		if params.Name == "ColorUp" then
			self:queuecommand("SetColor")
		end
		if params.Name == "ColorDown" then
			self:queuecommand("SetColor")
		end
	end,
	SetColorCommand = function(self)
		self:diffuse(color("#" .. table.concat(colorTable)))
	end
}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X - 60, SCREEN_CENTER_Y):zoom(0.8)
		end,
		OnCommand = function(self)
			self:settext("#")
			self:diffuse(color("#666666"))
		end
	}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y + 100):zoom(0.4)
		end,
		OnCommand = function(self)
			self:settextf(
				translated_info["Description"],
				selected[1],
				selected[2]
			)
			self:diffuse(color("#FFFFFF"))
		end
	}

for i = 1, 6 do
	t[#t + 1] = scroller(i)
end

return t
