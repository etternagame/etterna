local selected = getTableKeys()
local themeColor = colorConfig:get_data()[selected[1]][selected[2]]
local colorTable = {}
for i = 2, #themeColor do -- First char in string is a "#", ignore.
	colorTable[i - 1] = themeColor:sub(i, i)
end

local translated_info = {
	Title = THEME:GetString("ScreenColorEdit", "Title"),
	Description = THEME:GetString("ScreenColorEdit", "Description")
}

local t = Def.ActorFrame {
	CodeMessageCommand = function(self, params)
		if params.Name == "ColorCancel" then
			SCREENMAN:GetTopScreen():Cancel()
		end
		if params.Name == "ColorStart" then
			SCREENMAN:GetTopScreen():Cancel()
		end
	end
}


t[#t + 1] = Def.Quad {
	Name = "MainBG",
	InitCommand = function(self)
		self:xy(0, 0):halign(0):valign(0):zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(color("#000000EE"))
	end
}

t[#t + 1] = LoadActor("_frame")

t[#t + 1] = LoadFont("Common Large") .. {
	Name = "ScreenTitleText",
	InitCommand = function(self)
		self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("highlight")):settext(translated_info["Title"])
	end
}

return t