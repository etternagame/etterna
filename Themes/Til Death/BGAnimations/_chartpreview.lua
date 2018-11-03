-- all the preview stuff should be var'd and used consistently -mina
local noteField = false
local usingreverse = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
local prevrevY = 294
local prevZoom = 0.65
local musicratio = 1

-- hurrrrr nps quadzapalooza -mina
local wodth = 300
local hidth = 40
local yeet
local cd

local function UpdatePreviewPos(self)
	if noteField then
		local pos = SCREENMAN:GetTopScreen():GetPreviewNoteFieldMusicPosition() / musicratio
        self:GetChild("Pos"):zoomto(math.min(pos,wodth), hidth)
        yeet:xy(self:GetX()+wodth/2, self:GetY() + 80)
        if usingreverse then 
            yeet:y(self:GetY() + prevrevY)
		end
		self:queuecommand("Highlight")
	end
end

local function setUpPreviewNoteField() 
    yeet = SCREENMAN:GetTopScreen():CreatePreviewNoteField() 
    if yeet == nil then 
      return 
    end 
    yeet:xy(-500,-500)
    yeet:zoom(prevZoom) 
	MESSAGEMAN:Broadcast("NoteFieldVisible") 
  end 

local t = Def.ActorFrame {
	Name = "ChartPreview",
	InitCommand=function(self)
		self:visible(false)
        self:SetUpdateFunction(UpdatePreviewPos)
        cd = self:GetChild("ChordDensityGraph"):visible(true)
	end,
	CurrentSongChangedMessageCommand=function(self)
		self:GetChild("pausetext"):settext("")
	end,
	MouseRightClickMessageCommand=function(self)
		SCREENMAN:GetTopScreen():PausePreviewNoteField()
		if SCREENMAN:GetTopScreen():IsPreviewNoteFieldPaused() then 
			self:GetChild("pausetext"):settext("Paused")
		else 
			self:GetChild("pausetext"):settext("")
		end
	end,
    SetupNoteFieldCommand=function(self)
        setUpPreviewNoteField()
        noteField = true
    end,
	RefreshChartInfoMessageCommand = function(self)
		if GAMESTATE:GetCurrentSong() then
            musicratio = GAMESTATE:GetCurrentSong():GetLastSecond() / wodth
		end
	end,
	NoteFieldVisibleMessageCommand = function(self)
        self:visible(true)
		cd:visible(true):y(20)	-- need to control this manually -mina
		cd:GetChild("cdbg"):diffusealpha(0)	-- we want to use our position background for draw order stuff -mina
		self:queuecommand("PlayingSampleMusic") 
	end,
	DeletePreviewNoteFieldMessageCommand = function(self)
		self:visible(false)
		noteField = false
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:xy(wodth/2, SCREEN_HEIGHT/2) 
			self:zoomto(wodth*2/3, SCREEN_HEIGHT):diffuse(color("0.05,0.05,0.05,1"))
		end
	},
	LoadFont("Common Normal") .. {
		Name = "pausetext",
		InitCommand = function(self)
			self:xy(wodth/2, SCREEN_HEIGHT/2)
			self:settext(""):diffuse(color("0.8,0,0"))
		end
	},
	Def.Quad {
		Name = "PosBG",
		InitCommand = function(self)
			self:zoomto(wodth, hidth):halign(0):diffuse(color("1,1,1,1"))
		end,
		HighlightCommand = function(self)	-- use the bg for detection but move the seek pointer -mina 
			if isOver(self) then
				self:GetParent():GetChild("Seek"):visible(true)
				self:GetParent():GetChild("Seek"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX())
			else
				self:GetParent():GetChild("Seek"):visible(false)
			end
		end
	},
	Def.Quad {
		Name = "Pos",
		InitCommand = function(self)
			self:zoomto(0, hidth):diffuse(color("0,1,0,.5")):halign(0)
		end
	}
}

t[#t + 1] = LoadActor("_chorddensitygraph.lua")

-- more draw order shenanigans
t[#t + 1] = Def.Quad {
	Name = "Seek",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color("1,.2,.5,1")):halign(0.5)
	end,
    MouseLeftClickMessageCommand = function(self)
		if isOver(self) then
			SCREENMAN:GetTopScreen():SetPreviewNoteFieldMusicPosition(	self:GetX() * musicratio  )
		end
	end
}

return t