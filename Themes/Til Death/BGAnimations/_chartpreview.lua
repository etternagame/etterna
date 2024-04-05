-- all the preview stuff should be var'd and used consistently -mina
local prevZoom = 0.65
local musicratio = 1

local wodth = capWideScale(280, 300)
local hidth = 40
local yeet
local cd
local calcinfo

local yPos = Var("yPos")
local yPosReverse = Var("yPosReverse")
if not yPos then yPos = 55 end
if not yPosReverse then yPosReverse = 60 end

local translated_info = {
	Paused = THEME:GetString("ChartPreview", "Paused"),
	NPS = THEME:GetString("ChordDensityGraph", "NPS"),
	BPM = THEME:GetString("ChordDensityGraph", "BPM"),
}

local function UpdatePreviewPos(self)
	if not self:IsVisible() then return end
	local scrnm = SCREENMAN:GetTopScreen():GetName()
	local allowedScreens = {
		ScreenSelectMusic = true,
		ScreenNetSelectMusic = true,
	}

	if allowedScreens[scrnm] == true then
		local pos = SCREENMAN:GetTopScreen():GetSampleMusicPosition() / musicratio
		self:GetChild("Pos"):zoomto(math.min(pos,wodth), hidth)
		self:queuecommand("Highlight")

		-- calcdisplay position indicator (not the best place to put this but it works)
		local calcgraphpos = SCREENMAN:GetTopScreen():GetSampleMusicPosition() / musicratio
		local badorp = self:GetDescendant("notChordDensityGraph", "GraphPos")
		badorp:zoomto(math.min(calcgraphpos * capWideScale(300,450) / capWideScale(280,300), capWideScale(300,450)), hidth * 3):halign(0)
	end
end

local function updateCalcInfoDisplays(actor)
	if not calcinfo:GetVisible() then return end
	mx = INPUTFILTER:GetMouseX()
	px = actor:GetParent():GetX()
	sl1 = actor:GetParent():GetDescendant("notChordDensityGraph", "Seek1"):playcommand("UpdatePosition", {pos = mx, w = wodth, px=px})
	st1 = actor:GetParent():GetDescendant("notChordDensityGraph", "Seektext1"):playcommand("UpdatePosition", {pos = mx, w = wodth, px=px})
	st1:settextf("%0.2f", actor:GetParent():GetChild("Seek"):GetX() * musicratio /  getCurRateValue())
	sl1:visible(true)
	st1:visible(true)
end

local t = Def.ActorFrame {
	Name = "ChartPreview",
	InitCommand=function(self)
		self:visible(false)
        self:SetUpdateFunction(UpdatePreviewPos)
		calcinfo = self:GetChild("notChordDensityGraph"):visible(false):draworder(1000) -- actor for calcinfo
		cd = self:GetChild("ChordDensityGraph"):visible(false):draworder(1000)
	end,
	CurrentSongChangedMessageCommand=function(self)
		self:GetChild("pausetext"):settext("")
	end,
	CurrentStepsChangedMessageCommand = function(self)
		if GAMESTATE:GetCurrentSong() then
            musicratio = (GAMESTATE:GetCurrentSteps():GetFirstSecond() / getCurRateValue() + GAMESTATE:GetCurrentSteps():GetLengthSeconds()) / wodth * getCurRateValue()
		end
	end,
    SetupNoteFieldCommand=function(self)
		self:playcommand("NoteFieldVisible")
	end,
	ChartPreviewOffMessageCommand=function(self)
		self:SetUpdateFunction(nil)
	end,
	ChartPreviewOnMessageCommand=function(self)
		self:SetUpdateFunction(UpdatePreviewPos)
		self:GetChild("NoteField"):playcommand("LoadNoteData", {steps = GAMESTATE:GetCurrentSteps()})
	end,
	NoteFieldVisibleMessageCommand = function(self)
		self:visible(true)
		self:SetUpdateFunction(UpdatePreviewPos)
		cd:visible(true):y(20)				-- need to control this manually -mina
		cd:GetChild("cdbg"):diffusealpha(0)	-- we want to use our position background for draw order stuff -mina
		cd:queuecommand("GraphUpdate")		-- first graph will be empty if we dont force this on initial creation
	end,
	OptionsScreenClosedMessageCommand = function(self)
		local pOptions = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions()
		local usingscrollmod = false
		local usingreverse = pOptions:UsingReverse()
		local nf = self:GetChild("NoteField")
		if not nf then return end
		if pOptions:Split() ~= 0 or pOptions:Alternate() ~= 0 or pOptions:Cross() ~= 0 or pOptions:Centered() ~= 0 then
			usingscrollmod = true
		end

		nf:y(yPos * 2.85)
		if usingscrollmod then
			nf:y(yPos * 3.55)
		elseif usingreverse then
			nf:y(yPos * 2.85 + yPosReverse)
		end
	end,

	Def.NoteFieldPreview {
		Name = "NoteField",
		DrawDistanceBeforeTargetsPixels = 600,
		DrawDistanceAfterTargetsPixels = 0,
		--YReverseOffsetPixels = 100,

		BeginCommand = function(self)
			self:zoom(prevZoom):draworder(90)
			self:x(wodth/2)
			self:GetParent():SortByDrawOrder()
		end,
		CurrentStepsChangedMessageCommand = function(self, params)
			local steps = params.ptr
			-- only load new notedata if the preview is visible
			if self:GetParent():GetVisible() then
				self:playcommand("LoadNoteData", {steps = steps})
			end
		end,
		LoadNoteDataCommand = function(self, params)
			local steps = params.steps
			if steps ~= nil then
				self:LoadNoteData(steps)
			else
				self:LoadDummyNoteData()
			end
		end,
		CalcInfoOnMessageCommand = function(self)
			self:show_interval_bars(true)
		end,
		CalcInfoOffMessageCommand = function(self)
			self:show_interval_bars(false)
		end,
	},

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
	LoadFont("Common Large") .. {
		Name = "pausetext",
		InitCommand = function(self)
			self:xy(wodth/2, SCREEN_HEIGHT/2):draworder(900):zoom(0.5)
			self:settext(""):diffuse(color("0.8,0,0"))
			self:shadowlength(1):shadowcolor(0,0,0,1)
		end,
		NoteFieldVisibleMessageCommand = function(self)
			self:settext("")
		end,
		PreviewMusicStartedMessageCommand = function(self)
			self:playcommand("Set")
		end,
		SetCommand = function(self)
			if SCREENMAN:GetTopScreen():IsSampleMusicPaused() then
				self:settext(translated_info["Paused"])
			else
				self:settext("")
			end
		end,
		MusicPauseToggledMessageCommand = function(self)
			self:playcommand("Set")
		end,
	},
	Def.Quad {
		Name = "PosBG",
		InitCommand = function(self)
			--self:zoomto(wodth, hidth):halign(0):diffuse(color(".1,.1,.1,1")):draworder(900) -- alt bg for calc info
			self:zoomto(wodth, hidth):halign(0):diffuse(color("1,1,1,1")):draworder(900) -- cdgraph bg
		end,
		HighlightCommand = function(self)	-- use the bg for detection but move the seek pointer -mina
			if isOver(self) then
				local seek = self:GetParent():GetChild("Seek")
				local seektext = self:GetParent():GetChild("Seektext")
				local cdg = self:GetParent():GetChild("ChordDensityGraph")

				seek:visible(true)
				seektext:visible(true)
				seek:x(INPUTFILTER:GetMouseX() - self:GetParent():GetX())
				seektext:x(INPUTFILTER:GetMouseX() - self:GetParent():GetX() - 4)	-- todo: refactor this lmao -mina
				seektext:y(INPUTFILTER:GetMouseY() - self:GetParent():GetY())
				if cdg.npsVector ~= nil and #cdg.npsVector > 0 then
					local percent = clamp((INPUTFILTER:GetMouseX() - self:GetParent():GetX()) / wodth, 0, 1)
					local xtime = seek:GetX() * musicratio / getCurRateValue()
					local hoveredindex = clamp(math.ceil(cdg.finalNPSVectorIndex * percent), math.min(1, cdg.finalNPSVectorIndex), cdg.finalNPSVectorIndex)
					local hoverednps = cdg.npsVector[hoveredindex]
					local td = GAMESTATE:GetCurrentSteps():GetTimingData()
					local bpm = td:GetBPMAtBeat(td:GetBeatFromElapsedTime(seek:GetX() * musicratio)) * getCurRateValue()
					seektext:settextf("%0.2f\n%d %s\n%d %s", xtime, hoverednps, translated_info["NPS"], bpm, translated_info["BPM"])
				else
					seektext:settextf("%0.2f", seek:GetX() * musicratio / getCurRateValue())
				end

				updateCalcInfoDisplays(self)
			else
				self:GetParent():GetChild("Seektext"):visible(false)
				self:GetParent():GetChild("Seek"):visible(false)
				self:GetParent():GetDescendant("notChordDensityGraph", "Seektext1"):visible(false)
				self:GetParent():GetDescendant("notChordDensityGraph", "Seek1"):visible(false)
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
t[#t + 1] = LoadActor("_calcdisplay.lua")

-- more draw order shenanigans
t[#t + 1] = LoadFont("Common Normal") .. {
	Name = "Seektext",
	InitCommand = function(self)
		self:y(8):valign(0):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
	end
}

t[#t + 1] = UIElements.QuadButton(1, 1) .. {
	Name = "Seek",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
	end,
	MouseDownCommand = function(self, params)
		if params.event == "DeviceButton_left mouse button" then
			SCREENMAN:GetTopScreen():SetSampleMusicPosition( self:GetX() * musicratio )
		end
	end
}

return t
