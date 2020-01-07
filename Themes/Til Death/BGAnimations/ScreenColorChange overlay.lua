local curLevel = 1
local cursorIndex = {1, 1}
local selected = {"", ""}
local currentItems = {{}, {}}

local maxItems = 17
 -- odd only
local configData = colorConfig:get_data()

local frameX = {30, 180, 450}
local frameY = 60
local spacing = 20
local scale = 0.7

local function onEdge(size, center, radius)
	if center - radius < 1 then
		return true
	elseif center + radius > size then
		return true
	else
		return false
	end
end

-- Make groups
local function generateCategory()
	local visibleItems = {}
	for k, v in pairs(configData) do
		currentItems[1][#currentItems[1] + 1] = k
	end
	table.sort(currentItems[1])
	for i = 1, math.min(#currentItems[1], maxItems) do
		visibleItems[i] = currentItems[1][i]
	end
	selected[1] = currentItems[1][1]

	local t =
		Def.ActorFrame {
		RowChangedMessageCommand = function(self, params)
			if params.level == 1 then
				local radius = math.floor((maxItems - 1) / 2)
				if not onEdge(#currentItems[1], cursorIndex[1], radius) then
					for i = 1, math.min(maxItems) do
						visibleItems[i] = currentItems[1][cursorIndex[1] - radius - 1 + i]
					end
				end
			end
			selected[1] = currentItems[1][cursorIndex[1]]
			MESSAGEMAN:Broadcast("RowChanged2", {level = params.level})
		end
	}

	for k, v in pairs(visibleItems) do
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX[1], frameY + k * spacing)
					self:settext(THEME:GetString("ScreenColorChange", visibleItems[k]))
					self:zoom(scale)
					self:halign(0)
					self:queuecommand("UpdateColor")
					self:maxwidth((frameX[2]-frameX[1] - 5) / scale)
				end,
				RowChangedMessageCommand = function(self, params)
					if params.level == 1 then
						self:queuecommand("UpdateColor")
						self:settext(THEME:GetString("ScreenColorChange", visibleItems[k]))
					end
				end,
				ColChangedMessageCommand = function(self, params)
					if params.level == 1 then
						self:diffusealpha(1)
					else
						self:diffusealpha(0.5)
					end
				end,
				UpdateColorCommand = function(self)
					if visibleItems[k] == currentItems[1][cursorIndex[1]] then
						self:diffuse(getMainColor("highlight"))
					else
						self:diffuse(color("#FFFFFF"))
					end
				end
			}
	end

	return t
end

local function generateCategoryColors()
	local visibleItems = {}
	for k, v in pairs(configData[selected[1]]) do
		currentItems[2][#currentItems[2] + 1] = k
	end
	table.sort(currentItems[2])
	for i = 1, math.min(#currentItems[2], maxItems) do
		visibleItems[i] = currentItems[2][i]
	end

	selected[2] = currentItems[2][cursorIndex[2]]

	local t =
		Def.ActorFrame {
		RowChanged2MessageCommand = function(self, params)
			if params.level == 1 then
				currentItems[2] = {}
				visibleItems = {}
				for k, v in pairs(configData[selected[1]]) do
					currentItems[2][#currentItems[2] + 1] = k
				end
				table.sort(currentItems[2])
				for i = 1, math.min(#currentItems[2], maxItems) do
					visibleItems[i] = currentItems[2][i]
				end
			end

			local radius = math.floor((maxItems - 1) / 2)
			if not onEdge(#currentItems[2], cursorIndex[2], radius) then
				for i = 1, math.min(maxItems) do
					visibleItems[i] = currentItems[2][cursorIndex[2] - radius - 1 + i]
				end
			end
			selected[2] = currentItems[2][cursorIndex[2]]
		end
	}

	for i = 0, maxItems do
		-- Make names
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX[2], frameY + i * spacing)
					if visibleItems[i] ~= nil then
						self:settext(THEME:GetString("ScreenColorChange", visibleItems[i]))
					else
						self:visible(false)
					end
					self:zoom(scale)
					self:halign(0)
					self:queuecommand("UpdateColor")
					self:maxwidth((frameX[3]-frameX[2] - 5) / scale)
				end,
				RowChanged2MessageCommand = function(self, params)
					if params.level <= 2 then
						self:queuecommand("UpdateColor")
					end
				end,
				ColChangedMessageCommand = function(self, params)
					if params.level == 2 then
						self:diffusealpha(1)
					else
						self:diffusealpha(0.5)
					end
				end,
				UpdateColorCommand = function(self)
					if visibleItems[i] ~= nil then
						self:visible(true)
						self:settext(THEME:GetString("ScreenColorChange", visibleItems[i]))

						if visibleItems[i] == currentItems[2][cursorIndex[2]] then
							self:diffuse(getMainColor("highlight"))
						else
							self:diffuse(color("#FFFFFF"))
						end
						if curLevel == 2 then
							self:diffusealpha(1)
						else
							self:diffusealpha(0.5)
						end
					else
						self:visible(false)
					end
				end
			}

		-- Make Color hex value text
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX[3], frameY + i * spacing)
					if visibleItems[i] ~= nil then
						self:settext(configData[selected[1]][visibleItems[i]])
					else
						self:visible(false)
					end
					self:zoom(scale)
					self:halign(0)
					self:queuecommand("UpdateColor")
				end,
				RowChanged2MessageCommand = function(self, params)
					if params.level <= 2 then
						self:queuecommand("UpdateColor")
					end
				end,
				ColChangedMessageCommand = function(self, params)
					if params.level == 2 then
						self:diffusealpha(1)
					else
						self:diffusealpha(0.5)
					end
				end,
				UpdateColorCommand = function(self)
					if visibleItems[i] ~= nil then
						self:visible(true)
						if configData[selected[1]][visibleItems[i]] ~= nil then
							self:settext(string.upper(configData[selected[1]][visibleItems[i]]))
						else
							self:settext("dis is nil")
						end

						if visibleItems[i] == currentItems[2][cursorIndex[2]] then
							self:diffuse(color(configData[selected[1]][visibleItems[i]]))
						else
							self:diffuse(color("#FFFFFF"))
						end

						if curLevel == 2 then
							self:diffusealpha(1)
						else
							self:diffusealpha(0.5)
						end
					else
						self:visible(false)
					end
				end
			}
	end

	return t
end

local t =
	Def.ActorFrame {
	CodeMessageCommand = function(self, params)
		if params.Name == "ColorUp" then
			if curLevel == 1 then
				cursorIndex[curLevel] = math.max(1, cursorIndex[curLevel] - 1)
				cursorIndex[2] = 1
				cursorIndex[3] = 1
			elseif curLevel == 2 then
				cursorIndex[curLevel] = math.max(1, cursorIndex[curLevel] - 1)
			end

			MESSAGEMAN:Broadcast("RowChanged", {level = curLevel})
		elseif params.Name == "ColorDown" then
			if curLevel == 1 then
				cursorIndex[curLevel] = math.min(getTableSize(configData), cursorIndex[curLevel] + 1)
				cursorIndex[2] = 1
				cursorIndex[3] = 1
			elseif curLevel == 2 then
				cursorIndex[curLevel] = math.min(getTableSize(configData[selected[1]]), cursorIndex[curLevel] + 1)
			end

			MESSAGEMAN:Broadcast("RowChanged", {level = curLevel})
		elseif params.Name == "ColorLeft" then
			curLevel = math.max(1, curLevel - 1)
			MESSAGEMAN:Broadcast("ColChanged", {level = curLevel})
		elseif params.Name == "ColorRight" then
			curLevel = math.min(2, curLevel + 1)
			MESSAGEMAN:Broadcast("ColChanged", {level = curLevel})
		elseif params.Name == "ColorStart" then
			if curLevel == 1 then
				curLevel = math.min(2, curLevel + 1)
				MESSAGEMAN:Broadcast("ColChanged", {level = curLevel})
			elseif curLevel == 2 then
				setTableKeys(selected)
				SCREENMAN:AddNewScreenToTop("ScreenColorEdit")
			end
		elseif params.Name == "ColorCancel" then
			SCREENMAN:GetTopScreen():Cancel()
		end
	end
}

t[#t + 1] = LoadActor("_frame")
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("highlight"))
				:settext(THEME:GetString("ScreenColorChange", "Title"))
		end
	}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX[1], frameY):halign(0):valign(1):zoom(0.6):settext(THEME:GetString("ScreenColorChange", "Category"))
		end
	}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX[2], frameY):halign(0):valign(1):zoom(0.6):settext(THEME:GetString("ScreenColorChange", "Name"))
		end
	}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX[3], frameY):halign(0):valign(1):zoom(0.6):settext(THEME:GetString("ScreenColorChange", "Color"))
		end
	}

if configData ~= nil then
	t[#t + 1] = generateCategory()
	t[#t + 1] = generateCategoryColors()
end
return t
