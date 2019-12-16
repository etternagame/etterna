-- all the preview stuff should be var'd and used consistently -mina
local noteField = false
local prevZoom = 0.65
local musicratio = 1

-- hurrrrr nps quadzapalooza -mina
local wodth = capWideScale(280, 300)
local hidth = 40
local yeet
local cd

local yPos = Var("yPos")
local yPosReverse = Var("yPosReverse")
if not yPos then yPos = 55 end
if not yPosReverse then yPosReverse = 208 end

local translated_info = {
	Paused = THEME:GetString("ChartPreview", "Paused")
}

local function UpdatePreviewPos(self)
	if not self:IsVisible() then return end
	if noteField and yeet and SCREENMAN:GetTopScreen():GetName() == "ScreenSelectMusic" or 
	noteField and yeet and SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic" then
		local pos = SCREENMAN:GetTopScreen():GetPreviewNoteFieldMusicPosition() / musicratio
		self:GetChild("Pos"):zoomto(math.min(pos,wodth), hidth)
		self:queuecommand("Highlight")
	end
end

local memehamstermax
local function setUpPreviewNoteField() 
	yeet = SCREENMAN:GetTopScreen():CreatePreviewNoteField()
    if yeet == nil then 
      return 
	end 
	yeet:zoom(prevZoom):draworder(90)
	SCREENMAN:GetTopScreen():dootforkfive(memehamstermax)
	yeet = memehamstermax:GetChild("NoteField")
	yeet:x(wodth/2)
	memehamstermax:SortByDrawOrder()
	MESSAGEMAN:Broadcast("NoteFieldVisible")
end

local t = Def.ActorFrame {
	Name = "ChartPreview",
	InitCommand=function(self)
		self:visible(false)
		cd = self:GetChild("ChordDensityGraph"):visible(false):draworder(1000)
		memehamstermax = self
	end,
	CurrentSongChangedMessageCommand=function(self)
		self:GetChild("pausetext"):settext("")
		if GAMESTATE:GetCurrentSong() then
            musicratio = GAMESTATE:GetCurrentSong():GetLastSecond() / wodth
		end
	end,
	MouseRightClickMessageCommand=function(self)
		local tab = getTabIndex()
		-- the Score and Profile tabs have right click functionality
		-- so ignore right clicks if on those
		if tab ~= 2 and tab ~= 4 then
			SCREENMAN:GetTopScreen():PausePreviewNoteField()
			self:GetChild("pausetext"):playcommand("Set")
		end
	end,
    SetupNoteFieldCommand=function(self)
        setUpPreviewNoteField()
        noteField = true
	end,
	hELPidontDNOKNOWMessageCommand=function(self)
		SCREENMAN:GetTopScreen():DeletePreviewNoteField(self)
		self:SetUpdateFunction(nil)
	end,
	ChartPreviewOffMessageCommand=function(self)
		self:SetUpdateFunction(nil)
	end,
	ChartPreviewOnMessageCommand=function(self)
		self:SetUpdateFunction(UpdatePreviewPos)
	end,
	NoteFieldVisibleMessageCommand = function(self)
		self:visible(true)
		self:SetUpdateFunction(UpdatePreviewPos)
		cd:visible(true):y(20)				-- need to control this manually -mina
		cd:GetChild("cdbg"):diffusealpha(0)	-- we want to use our position background for draw order stuff -mina
		cd:queuecommand("GraphUpdate")		-- first graph will be empty if we dont force this on initial creation
	end,
	OptionsScreenClosedMessageCommand = function(self)
		local rev = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
		if self:GetChild("NoteField") ~= nil then
			if not rev then
				self:GetChild("NoteField"):y(yPos * 1.5)
			else
				self:GetChild("NoteField"):y(yPos * 1.5 + yPosReverse)
			end
		end
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:xy(wodth/2, SCREEN_HEIGHT/2) 
			self:diffuse(color("0.05,0.05,0.05,1"))
		end,
		CurrentStyleChangedMessageCommand=function(self)
			local cols = GAMESTATE:GetCurrentStyle():ColumnsPerPlayer()
			self:zoomto(48 * cols, SCREEN_HEIGHT)
		end
	},
	LoadFont("Common Normal") .. {
		Name = "pausetext",
		InitCommand = function(self)
			self:xy(wodth/2, SCREEN_HEIGHT/2)
			self:settext(""):diffuse(color("0.8,0,0"))
		end,
		NoteFieldVisibleMessageCommand = function(self)
			self:settext("")
		end,
		PreviewMusicStartedMessageCommand = function(self)
			self:playcommand("Set")
		end,
		SetCommand = function(self)
			if SCREENMAN:GetTopScreen():IsPreviewNoteFieldPaused() then 
				self:settext(translated_info["Paused"])
			else 
				self:settext("")
			end
		end
	},
	Def.Quad {
		Name = "PosBG",
		InitCommand = function(self)
			self:zoomto(wodth, hidth):halign(0):diffuse(color("1,1,1,1")):draworder(900)
		end,
		HighlightCommand = function(self)	-- use the bg for detection but move the seek pointer -mina 
			if isOver(self) then
				self:GetParent():GetChild("Seek"):visible(true)
				self:GetParent():GetChild("Seektext"):visible(true)
				self:GetParent():GetChild("Seek"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX())
				self:GetParent():GetChild("Seektext"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX() - 4)	-- todo: refactor this lmao -mina
				self:GetParent():GetChild("Seektext"):y(INPUTFILTER:GetMouseY() - self:GetParent():GetY())
				self:GetParent():GetChild("Seektext"):settextf("%0.2f", self:GetParent():GetChild("Seek"):GetX() * musicratio /  getCurRateValue())
			else
				self:GetParent():GetChild("Seektext"):visible(false)
				self:GetParent():GetChild("Seek"):visible(false)
			end
		end
	},
	Def.Quad {
		Name = "Pos",
		InitCommand = function(self)
			self:zoomto(0, hidth):diffuse(color("0,1,0,.5")):halign(0):draworder(900)
		end
	}
}

t[#t + 1] = LoadActor("_chorddensitygraph.lua")

-- more draw order shenanigans
t[#t + 1] = LoadFont("Common Normal") .. {
	Name = "Seektext",
	InitCommand = function(self)
		self:y(8):valign(1):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
	end
}

t[#t + 1] = Def.Quad {
	Name = "Seek",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) then
			SCREENMAN:GetTopScreen():SetPreviewNoteFieldMusicPosition(	self:GetX() * musicratio  )
		end
	end
}

return t