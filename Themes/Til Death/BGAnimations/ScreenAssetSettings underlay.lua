-- shamelessly adapted from spawncamping-wallhack
local top
local profile = PROFILEMAN:GetProfile(PLAYER_1)

local curType = 1
local assetTypes = {}
local translated_assets = {}
for k,v in pairs(assetFolders) do
	assetTypes[curType] = k
	translated_assets[k] = THEME:GetString("ScreenAssetSettings", k)
    curType = curType + 1
end

curType = 2

local maxPage = 1
local curPage = 1
local maxRows = 5
local maxColumns = 5
local curIndex = 1
local selectedIndex = 0
local GUID = profile:GetGUID()
local curPath = ""
local lastClickedIndex = 0

local assetTable = {}

local frameWidth = SCREEN_WIDTH - 20
local frameHeight = SCREEN_HEIGHT - 40
local squareWidth = 50
local judgmentWidth = 125
local assetWidth = squareWidth
local assetHeight = 50
local assetXSpacing = (frameWidth + assetWidth/2) / (maxColumns + 1)
local assetYSpacing = (frameHeight - 20) / (maxRows + 1)

local co -- for async loading images

local function findIndexForCurPage()
	local type = assetTypes[curType]
	for i = 1+((curPage-1)*maxColumns*maxRows), 1+((curPage)*maxColumns*maxRows) do
		if assetTable[i] == nil then return nil end
		if assetFolders[type] .. assetTable[i] == curPath then
			return i
		end
	end
end

local function findPickedIndexForCurPage()
	local type = assetTypes[curType]
	for i = 1, #assetTable do
		if assetTable[i] == nil then return nil end
		if assetFolders[type] .. assetTable[i] == selectedPath then
			return i
		end
	end
end

local function isImage(filename)
	local extensions = {".png", ".jpg", "jpeg"} -- lazy list
	local ext = string.sub(filename, #filename-3)
	for i=1, #extensions do
		if extensions[i] == ext then return true end
	end
	return false
end

local function isAudio(filename)
	local extensions = {".wav", ".mp3", ".ogg", ".mp4"} -- lazy to check and put in names
	local ext = string.sub(filename, #filename-3)
	for i=1, #extensions do
		if extensions[i] == ext then return true end
	end
	return false
end

local function getImagePath(path, assets) -- expecting a table of asset paths where fallbacks are default
	for i=1, #assets do
		if isImage(assets[i]) then
			return path .. "/" .. assets[i]
		end
	end
	return assetsFolder .. assetFolders[assetTypes[curType]] .. getDefaultAssetByType(assetType[curType]) .. "/default.png"
end
local function getSoundPath(path, assets) -- expecting a table of asset paths where fallbacks are default
	for i=1, #assets do
		if isAudio(assets[i]) then
			return path .. "/" .. assets[i]
		end
	end
	return assetsFolder .. assetFolders[assetTypes[curType]] .. getDefaultAssetByType(assetTypes[curType]) .. "/default.ogg"
end

local function containsDirsOnly(dirlisting)
	if #dirlisting == 0 then return true end
	for i=1, #dirlisting do
		if isImage(dirlisting[i]) or isAudio(dirlisting[i]) then
			return false
		end
	end
	return true
end

local function loadAssetTable() -- load asset table for current type
	local type = assetTypes[curType]
	curPath = getAssetByType(type, GUID)
	selectedPath = getAssetByType(type, GUID)
	local dirlisting = FILEMAN:GetDirListing(assetFolders[type])
	if containsDirsOnly(dirlisting) then
		assetTable = dirlisting
	else
		assetTable = filter(isImage, dirlisting)
	end
	maxPage = math.max(1, math.ceil(#assetTable/(maxColumns * maxRows)))
	local ind = findIndexForCurPage()
	local pickind = findPickedIndexForCurPage()
	if pickind ~= nil then selectedIndex = pickind end
	if ind ~= nil then curIndex = ind end
end

local function confirmPick() -- select the asset in the current index for use ingame
	if curIndex == 0 then return end
	local type = assetTypes[curType]
	local name = assetTable[lastClickedIndex+((curPage-1)*maxColumns*maxRows)]
	if name == nil then return end
	local path = assetFolders[type] .. name
	curPath = path
	selectedPath = path
	selectedIndex = curIndex

	setAssetsByType(type, GUID, path)

	MESSAGEMAN:Broadcast("PickChanged")
end

local function updateImages() -- Update all image actors (sprites)
	loadAssetTable()
	MESSAGEMAN:Broadcast("UpdatingAssets", {name = assetTypes[curType]})
    for i=1, math.min(maxRows * maxColumns, #assetTable) do
        MESSAGEMAN:Broadcast("UpdateAsset", {index = i})
        coroutine.yield()
    end
	MESSAGEMAN:Broadcast("UpdateFinished")
end

local function loadAssetType(n) -- move and load asset type forward/backward
	if n < 0 then n = 0 end
	if n > #assetTypes then n = #assetTypes end
	lastClickedIndex = 0
	curPage = 1
	curType = n
	co = coroutine.create(updateImages)
end

local function getIndex() -- Get cursor index
	local out = ((curPage-1) * maxColumns * maxRows) + curIndex
    return out
end

local function getSelectedIndex() -- Get cursor index
	local out = ((curPage-1) * maxColumns * maxRows) + selectedIndex
    return out
end

local function movePage(n) -- Move n pages forward/backward
    local nextPage = curPage + n
    if nextPage > maxPage then
        nextPage = maxPage
    elseif nextPage < 1 then
        nextPage = 1
    end

    -- This loads all images again if we actually move to a new page.
    if nextPage ~= curPage then
		curIndex = n < 0 and math.min(#assetTable, maxRows * maxColumns) or 1
		lastClickedIndex = 0
        curPage = nextPage
        MESSAGEMAN:Broadcast("PageMoved",{index = curIndex, page = curPage})
        co = coroutine.create(updateImages)
    end
end

local function moveCursor(x, y) -- move the cursor
    local move = x + y * maxColumns
	local nextPage = curPage
	local oldIndex = curIndex

    if curPage > 1 and curIndex == 1 and move < 0 then
        curIndex = math.min(#assetTable, maxRows * maxColumns)
        nextPage = curPage - 1
    elseif curPage < maxPage and curIndex == maxRows * maxColumns and move > 0 then
        curIndex = 1
        nextPage = curPage + 1
    else
        curIndex = curIndex + move
        if curIndex < 1 then
            curIndex = 1
        elseif curIndex > math.min(maxRows * maxColumns, #assetTable - (maxRows * maxColumns * (curPage-1))) then
            curIndex = math.min(maxRows * maxColumns, #assetTable - (maxRows * maxColumns * (curPage-1)))
        end
	end
	lastClickedIndex = curIndex
	if curPage == nextPage then
		MESSAGEMAN:Broadcast("CursorMoved",{index = curIndex, prevIndex = oldIndex})
	else
		curPage = nextPage
		MESSAGEMAN:Broadcast("PageMoved",{index = curIndex, page = curPage})
		co = coroutine.create(updateImages)
	end
end

-- more code lifted straight from scwh
local TAB = {
	choices = {},
	width = 100,
	height = 20
}

function TAB.new(self, choices)
	TAB.choices = choices
	TAB.width = math.min(100, SCREEN_WIDTH*2/3 / #choices)

	return self
end

function TAB.makeTabActors(tab)
	local t = Def.ActorFrame{}

	for i,v in pairs(tab.choices) do

		t[#t+1] = Def.Quad {
		InitCommand = function(self)
			self:halign(0)
			self:zoomto(tab.width, tab.height)
			self:x(tab.width*(i-1))
			self:diffuse(color("#666666"))
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				MESSAGEMAN:Broadcast("TabPressed",{name = v, index=i})
				self:finishtweening()
				self:smooth(0.1)
				self:diffusealpha(0.7)
				loadAssetType(i)
			end
		end,
		MouseRightClickMessageCommand = function(self)
			SCREENMAN:GetTopScreen():Cancel()
		end,
		TabPressedMessageCommand = function(self, params)
			if params.name ~= v then
				self:finishtweening()
				self:smooth(0.1)
				self:diffusealpha(1)
			end
		end,
		UpdatingAssetsMessageCommand = function(self)
			if curType == i then
				self:finishtweening()
				self:smooth(0.1)
				self:diffusealpha(0.7)
			else
				self:finishtweening()
				self:smooth(0.1)
				self:diffusealpha(1)
			end
		end

	}

		t[#t+1] = LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x((tab.width/2)+(tab.width*(i-1)))
				self:maxwidth(tab.width/0.25)
				self:zoom(0.25)
				self:settext(v)
			end
		}
	end

	return t
end
---

local function assetBox(i)
    local name = assetTable[i]
    local t = Def.ActorFrame {
        Name = tostring(i),
        InitCommand = function(self)
            self:x((((i-1) % maxColumns)+1)*assetXSpacing)
            self:y(((math.floor((i-1)/maxColumns)+1)*assetYSpacing)-10+50)
            self:diffusealpha(0)
        end,
        PageMovedMessageCommand = function(self)
			self:finishtweening()
			self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
			self:diffusealpha(0)
        end,
        UpdateAssetMessageCommand = function(self, params)
			if params.index == i then
				if i+((curPage-1)*maxColumns*maxRows) > #assetTable then
					self:finishtweening()
					self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
					self:diffusealpha(0)
				else
					local type = assetTypes[curType]
					name = assetFolders[type] .. assetTable[i+((curPage-1)*maxColumns*maxRows)]
					if name == curPath then
						curIndex = i
					end

					if curType == 3 then
						assetWidth = judgmentWidth
					else
						assetWidth = squareWidth
					end

					-- Load the asset image
					self:GetChild("Image"):playcommand("LoadAsset")
					self:GetChild("Sound"):playcommand("LoadAsset")
					self:GetChild("SelectedAssetIndicator"):playcommand("Set")
					if i == curIndex then
						self:GetChild("Image"):finishtweening()
						self:GetChild("Image"):zoomto(assetHeight+8,assetWidth+8)
						self:GetChild("Border"):zoomto(assetHeight+12,assetWidth+12)
						self:GetChild("Border"):diffuse(getMainColor("highlight")):diffusealpha(0.8)
					else
						self:GetChild("Image"):zoomto(assetHeight,assetWidth)
						self:GetChild("Border"):zoomto(assetHeight+4,assetWidth+4)
						self:GetChild("Border"):diffuse(getMainColor("positive")):diffusealpha(0)
					end

					self:y(((math.floor((i-1)/maxColumns)+1)*assetYSpacing)-10+50)
					self:finishtweening()
					self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
					self:diffusealpha(1)
					self:y((math.floor((i-1)/maxColumns)+1)*assetYSpacing+50)
							
				end
            end
		end,
		UpdateFinishedMessageCommand = function(self)
			if assetTable[i+((curPage-1)*maxColumns*maxRows)] == nil then
				self:finishtweening()
				self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
				self:diffusealpha(0)
			end
			if curType == 3 then
				MESSAGEMAN:Broadcast("CursorMoved",{index = findPickedIndexForCurPage()})
			end
		end
	}
	
	t[#t+1] = Def.Quad {
        Name = "SelectedAssetIndicator",
        InitCommand = function(self)
            self:zoomto(assetWidth+14, assetHeight+14)
			self:diffuse(color("#AAAAAA")):diffusealpha(0)
		end,
		SetCommand = function(self)
			self:zoomto(assetWidth+14, assetHeight+14)
			self:finishtweening()
			if selectedPath == name then
				self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
				self:diffusealpha(0.8)
			else
				self:smooth(0.2)
				self:diffusealpha(0)
			end
		end,
		PageMovedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		PickChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
    }

    t[#t+1] = Def.Quad {
        Name = "Border",
        InitCommand = function(self)
            self:zoomto(assetWidth+4, assetHeight+4)
            self:diffuse(getMainColor("positive")):diffusealpha(0.8)
		end,
		SelectCommand = function(self)
			self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
			self:zoomto(assetWidth+12, assetHeight+12)
			self:diffuse(getMainColor("highlight")):diffusealpha(0.8)
		end,
		DeselectCommand = function(self)
			self:smooth(0.2)
			self:zoomto(assetWidth+4, assetHeight+4)
			self:diffuse(getMainColor("positive")):diffusealpha(0)
		end,
        CursorMovedMessageCommand = function(self, params)
			self:finishtweening()
			if params.index == i then
				self:playcommand("Select")
			else
				self:playcommand("Deselect")
			end
		end,
		PageMovedMessageCommand = function(self, params)
			self:finishtweening()
			if params.index == i then
				self:playcommand("Select")
			else
				self:playcommand("Deselect")
			end
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and assetTable[i+((curPage-1)*maxColumns*maxRows)] ~= nil then
				if lastClickedIndex == i then
					confirmPick()
				end
				local prev = curIndex
				lastClickedIndex = i
				curIndex = i
				MESSAGEMAN:Broadcast("CursorMoved",{index = i, prevIndex = prev})
			end
		end
	}
    
    t[#t+1] = Def.Sprite {
        Name = "Image",
		LoadAssetCommand = function(self)
			local assets = findAssetsForPath(name)
			if #assets > 1 then
				local image = getImagePath(name, assets)
				self:LoadBackground(image)
			else
				self:LoadBackground(name)
			end
        end,
		CursorMovedMessageCommand = function(self, params)
			self:finishtweening()
			if params.index == i then
				self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
				self:zoomto(assetWidth+8, assetHeight+8)
			else
				self:smooth(0.2)
				self:zoomto(assetWidth, assetHeight)
			end
		end,
		PageMovedMessageCommand = function(self, params)
			self:finishtweening()
			if params.index == i then
				self:tween(0.5,"TweenType_Bezier",{0,0,0,0.5,0,1,1,1})
				self:zoomto(assetWidth+8, assetHeight+8)
			else
				self:smooth(0.2)
				self:zoomto(assetWidth, assetHeight)
			end
		end
	}
	
	t[#t+1] = Def.Sound {
		Name = "Sound",
		LoadAssetCommand = function(self)
			local assets = findAssetsForPath(name)
			if #assets > 1 then
				local soundpath = getSoundPath(name, assets)
				self:load(soundpath)
			else
				self:load("")
			end

		end,
		CursorMovedMessageCommand = function(self, params)
			if params.index == i and curType == 1 and params.prevIndex ~= i then
				self:play()
			end
		end
	}
    
    return t
end



local function highlight(self)
	self:queuecommand("Highlight")
end

local function mainContainer()
	local fontScale = 0.5
	local smallFontScale = 0.35
	local tinyFontScale = 0.2
	local fontRow1 = -frameHeight/2+20
	local fontRow2 = -frameHeight/2+45
	local fontSpacing = 15

	local t = Def.ActorFrame {
		InitCommand = function(self)
			self:SetUpdateFunction(highlight)
		end
	}

    t[#t+1] = Def.Quad {
        InitCommand = function(self)
            self:zoomto(frameWidth, frameHeight)
            self:diffuse(color("#333333")):diffusealpha(0.8)
        end
	}
	
	t[#t+1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(fontScale)
			self:halign(0)
			self:xy(-frameWidth/2 + fontSpacing, fontRow1)
			self:settext(THEME:GetString("ScreenAssetSettings", "Title"))
		end
	}

	--[[
	-- instructions
	t[#t+1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(smallFontScale)
			self:xy(0, frameHeight/2 - 15)
			self:settext("Arrows to move       Enter to select       Mouse support enabled")
		end
	}]]

	t[#t+1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(smallFontScale)
			self:xy(-frameWidth/2 + fontSpacing + 35, fontRow2 + 5)
			self:settext("")
		end,
		SetCommand = function(self)
			local cur = getIndex()
			local max = #assetTable
			self:settext(cur .. "/" .. max)
		end,
		UpdateFinishedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CursorMovedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

	t[#t+1] = LoadFont("Common Large") .. {
		Name = "CurrentPath",
		InitCommand = function(self)
			self:zoom(smallFontScale)
			self:halign(0)
			self:xy(-frameWidth/2 + fontSpacing, frameHeight/2 - 15)
			self:maxwidth((SCREEN_CENTER_X - TAB.width*#assetTypes/2 - 40)/smallFontScale)
		end,
		SetCommand = function(self)
			local type = assetTable[getIndex()]
			if type == nil then
				self:settext("")
			else
				self:settext(type:gsub("^%l", string.upper))
			end
		end,
		CursorMovedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		UpdateFinishedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

	t[#t+1] = LoadFont("Common Large") .. {
		Name = "SelectedPath",
		InitCommand = function(self)
			self:zoom(smallFontScale)
			self:halign(0)
			self:xy(TAB.width*#assetTypes/2 + 15, frameHeight/2 - 15)
			self:maxwidth((SCREEN_CENTER_X - TAB.width*#assetTypes/2 - 40)/smallFontScale)
		end,
		SetCommand = function(self)
			local type = assetTable[selectedIndex]
			if type == nil then
				self:settext("")
			else
				self:settext(type:gsub("^%l", string.upper))
			end
		end,
		PickChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		UpdateFinishedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

	t[#t+1] = LoadFont("Common Large") .. {
		Name = "AssetType",
		InitCommand = function(self)
			self:zoom(fontScale)
			self:xy(0, fontRow1)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local type = translated_assets[assetTypes[curType]]
			self:settext(type)
		end,
		UpdatingAssetsMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}
--[[
	t[#t+1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(smallFontScale)
			self:xy(-100, frameHeight/2 - 18)
			self:settext("Prev")
		end,
		HighlightCommand = function(self)
			if isOver(self) then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.6)
			end
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				loadAssetType(-1)
			end
		end
	}

	t[#t+1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(smallFontScale)
			self:xy(100, frameHeight/2 - 18)
			self:settext("Next")
		end,
		HighlightCommand = function(self)
			if isOver(self) then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.6)
			end
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				loadAssetType(1)
			end
		end
	}]]

    return t
end


local function input(event)
	if event.type ~= "InputEventType_Release" then
		if event.button == "Back" then
			SCREENMAN:GetTopScreen():Cancel()
		end

		if event.button == "Start" then
			confirmPick()
		end

		if event.button == "Left" or event.button == "MenuLeft" then
			moveCursor(-1, 0)
		end

		if event.button == "Right" or event.button == "MenuRight" then
			moveCursor(1, 0)
		end

		if event.button == "Up" or event.button == "MenuUp" then
			moveCursor(0, -1)
		end

		if event.button == "Down" or event.button == "MenuDown" then
			moveCursor(0, 1)
		end

		if event.button == "EffectUp" then
			loadAssetType(curType + 1)
		end

		if event.button == "EffectDown" then
			loadAssetType(curType - 1)
        end
	end
	if event.type == "InputEventType_FirstPress" then
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
			MESSAGEMAN:Broadcast("MouseRightClick")
		
		elseif event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
			movePage(-1)
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then
			movePage(1)
		end
	end

	return false

end



local function update(self, delta)
	if coroutine.status(co) ~= "dead" then
		coroutine.resume(co)
	end
end

local t = Def.ActorFrame {
	BeginCommand = function(self)
		SCREENMAN:set_input_redirected(PLAYER_1, true)
        top = SCREENMAN:GetTopScreen()
        top:AddInputCallback(input)
        co = coroutine.create(updateImages)
        self:SetUpdateFunction(update)
	end

}

t[#t+1] = mainContainer() .. {
    InitCommand = function(self)
        self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
    end
}

local l = 1
local capTypes = {}
for k,v in pairs(translated_assets) do
	capTypes[l] = v
    l = l+1
end
local typeTabs = TAB:new(capTypes)
t[#t+1] = typeTabs:makeTabActors() .. {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X - TAB.width*#assetTypes/2, SCREEN_CENTER_Y + frameHeight/2 - TAB.height/2)
	end
}

for i=1, maxRows * maxColumns do
    t[#t+1] = assetBox(i)
end

return t