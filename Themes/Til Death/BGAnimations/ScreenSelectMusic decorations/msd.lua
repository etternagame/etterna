--Local vars
local update = false
local steps
local song
local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320, 400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]
local greatest = 0
local steps
local meter = {}
meter[1] = 0.00

local noteField = false

-- all the preview stuff should be var'd and used consistently -mina
local prevX = 200
local prevY = 125
local prevZoom = 0.65
local musicratio = 1

local function isOver(element)
	if element:GetParent():GetParent():GetVisible() == false then
		return false
	end
	if element:GetParent():GetVisible() == false then
		return false
	end
	if element:GetVisible() == false then
		return false
	end
	local x = getTrueX(element)
	local y = getTrueY(element)
	local hAlign = element:GetHAlign()
	local vAlign = element:GetVAlign()
	local w = element:GetZoomedWidth()
	local h = element:GetZoomedHeight()
 	local mouseX = INPUTFILTER:GetMouseX()
	local mouseY = INPUTFILTER:GetMouseY()
 	local withinX = (mouseX >= (x - (hAlign * w))) and (mouseX <= ((x + w) - (hAlign * w)))
	local withinY = (mouseY >= (y - (vAlign * h))) and (mouseY <= ((y + h) - (vAlign * h)))
 	return (withinX and withinY)
end
local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end
local function highlight(self)
	self:queuecommand("Highlight")
end

-- Set up the values for the preview notefield here
local function setUpPreviewNoteField()
	local yeet = SCREENMAN:GetTopScreen():CreatePreviewNoteField()
	if yeet == nil then
		return
	end

	--SCREENMAN:AddNewScreenToTop("ScreenChartPreviewNoteField")
	yeet:x(prevX)
	yeet:y(prevY)
	yeet:zoom(prevZoom)
	MESSAGEMAN:Broadcast("NoteFieldVisible")
	noteField = true
end

local function input(event)
	if not noteField then
		return false
	end
	if event.DeviceInput.button == "DeviceButton_right mouse button" then	-- removed left click because using it to seek
		if event.type == "InputEventType_FirstPress" then
			SCREENMAN:GetTopScreen():PausePreviewNoteField()
		end
	end
	if event.DeviceInput.button == "DeviceButton_space" then	-- temp cancel command -mina
		if event.type == "InputEventType_Release"  and NoteField then
			MESSAGEMAN:Broadcast("DeletePreviewNoteField")
		end
	end
	if event.type ~= "InputEventType_Release" then
		if event.GameButton == "Back" or event.GameButton == "Start" then
			MESSAGEMAN:Broadcast("DeletePreviewNoteField")
		end
	end
	return false
end

--Actor Frame
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
		self:SetUpdateFunction(highlight)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
		noteField = false
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 1 then
			self:queuecommand("On")
			self:visible(true)
			song = GAMESTATE:GetCurrentSong()
			steps = GAMESTATE:GetCurrentSteps(PLAYER_1)

			--Find max MSD value, store MSD values in meter[]
			-- I plan to have c++ store the highest msd value as a separate variable to aid in the filter process so this won't be needed afterwards - mina
			greatest = 0
			if song and steps then
				for i = 1, #ms.SkillSets do
					meter[i + 1] = steps:GetMSD(getCurRateValue(), i)
					if meter[i + 1] > meter[greatest + 1] then
						greatest = i
					end
				end
			end

			MESSAGEMAN:Broadcast("UpdateMSDInfo")
			update = true
		else
			self:queuecommand("Off")
			MESSAGEMAN:Broadcast("DeletePreviewNoteField")
			update = false
		end
	end,
	CurrentRateChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	RefreshChartInfoMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

--BG quad
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
	end
}

--Skillset label function
local function littlebits(i)
	local t =
		Def.ActorFrame {
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 35, frameY + 120 + 22 * i):halign(0):valign(0):zoom(0.5):maxwidth(110 / 0.6)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					--skillset name
					if song and steps then
						self:settext(ms.SkillSets[i] .. ":")
					else
						self:settext("")
					end
					--highlight
					if greatest == i then
						self:diffuse(getMainColor("positive"))
					else
						self:diffuse(getMainColor("negative"))
					end
					--If negative BPM empty label
					if steps and steps:GetTimingData():HasWarps() then
						self:settext("")
					end
				end,
				UpdateMSDInfoCommand = function(self)
					self:queuecommand("Set")
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 225, frameY + 120 + 22 * i):halign(1):valign(0):zoom(0.5):maxwidth(110 / 0.6)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					if song and steps then
						self:settextf("%05.2f", meter[i + 1])
						self:diffuse(byMSD(meter[i + 1]))
					else
						self:settext("")
					end
					--If negative BPM empty label
					if steps and steps:GetTimingData():HasWarps() then
						self:settext("")
					end
				end,
				CurrentRateChangedMessageCommand = function(self)
					self:queuecommand("Set")
				end,
				UpdateMSDInfoCommand = function(self)
					self:queuecommand("Set")
				end
			}
	}
	return t
end

--Song Title
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, offsetY):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(
			0.5
		)
	end
}
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 5, frameY + offsetY - 9):zoom(0.6):halign(0):diffuse(getMainColor("positive")):settext(
				"MSD Breakdown (Wip)"
			)
		end
	}
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 5, frameY + 35):zoom(0.6):halign(0):diffuse(getMainColor("positive")):maxwidth(
				SCREEN_CENTER_X / 0.7
			)
		end,
		SetCommand = function(self)
			if song then
				self:settext(song:GetDisplayMainTitle())
			else
				self:settext("")
			end
		end,
		UpdateMSDInfoCommand = function(self)
			self:queuecommand("Set")
		end
	}

-- Music Rate Display
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + frameWidth - 100, frameY + offsetY + 65):visible(true):halign(0):zoom(0.4):maxwidth(
				capWideScale(get43size(360), 360) / capWideScale(get43size(0.45), 0.45)
			)
		end,
		SetCommand = function(self)
			self:settext(getCurRateDisplayString())
		end,
		CurrentRateChangedCommand = function(self)
			self:queuecommand("set")
		end
	}

--Difficulty
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "StepsAndMeter",
		InitCommand = function(self)
			self:xy(frameX + frameWidth - offsetX, frameY + offsetY + 50):zoom(0.5):halign(1)
		end,
		SetCommand = function(self)
			steps = GAMESTATE:GetCurrentSteps(pn)
			if steps ~= nil then
				local diff = getDifficulty(steps:GetDifficulty())
				local stype = ToEnumShortString(steps:GetStepsType()):gsub("%_", " ")
				local meter = steps:GetMeter()
				if update then
					self:settext(stype .. " " .. diff .. " " .. meter)
					self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(), steps:GetDifficulty())))
				end
			end
		end,
		ScoreUpdateMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

--NPS
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "NPS",
		InitCommand = function(self)
			self:xy(frameX + frameWidth - 15, frameY + 60):zoom(0.4):halign(1)
		end,
		SetCommand = function(self)
			steps = GAMESTATE:GetCurrentSteps(pn)
			--local song = GAMESTATE:GetCurrentSong()
			local notecount = 0
			local length = 1
			if steps ~= nil and song ~= nil and update then
				length = song:GetStepsSeconds()
				notecount = steps:GetRadarValues(pn):GetValue("RadarCategory_Notes")
				self:settext(string.format("%0.2f Average NPS", notecount / length * getCurRateValue()))
				self:diffuse(Saturation(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(), steps:GetDifficulty())), 0.3))
			else
				self:settext("0.00 Average NPS")
			end
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

--Negative BPMs label
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 45, frameY + 135):zoom(0.8):halign(0):diffuse(getMainColor("negative")):settext("Negative Bpms")
		end,
		SetCommand = function(self)
			if steps and steps:GetTimingData():HasWarps() then
				self:settext("Negative Bpms")
			else
				self:settext("")
			end
		end,
		UpdateMSDInfoCommand = function(self)
			self:queuecommand("Set")
		end
	}

--Skillset labels
for i = 1, #ms.SkillSets do
	t[#t + 1] = littlebits(i)
end

--Chart Preview Button
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "PreviewViewer",
		InitCommand = function(self)
			self:xy(frameX + 5, frameY + 75)
			self:zoom(0.5)
			self:halign(0)
			self:settext("Enable Preview")
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand = function(self)
			if getTabIndex() == 1 and isOver(self) then
				setUpPreviewNoteField()
			end
		end
	}
t[#t + 1] =
	Def.Quad {
	Name = "PreviewNoteFieldBackground",
	InitCommand = function(self)
		self:xy(prevX, SCREEN_HEIGHT/2) 
		self:zoomto(200, SCREEN_HEIGHT):diffuse(color("0.05,0.05,0.05,0.05")):diffusealpha(1)
		self:visible(false)
	end,
	NoteFieldVisibleMessageCommand = function(self)
		self:visible(true)
	end,
	DeletePreviewNoteFieldMessageCommand = function(self)
		self:visible(false)
		noteField = false
	end
}

-- hurrrrr nps quadzapalooza -mina
local imcrazy = 500
local wodth = 300
local hidth = 40
local toot = Def.ActorFrame {
	Name = "npsyo",
	InitCommand=function(self)
		self:xy(40,-170)
		self:visible(false)
	end,
	NoteFieldVisibleMessageCommand = function(self)
		self:visible(true)
		self:queuecommand("PlayingSampleMusic")
	end,
	DeletePreviewNoteFieldMessageCommand = function(self)
		self:visible(false)
	end,
	PlayingSampleMusicMessageCommand = function(self)
		if steps and noteField then
			local moot = steps:GetNPSVector()
			local joot = steps:GetCPSVector(2)
			local hoot = steps:GetCPSVector(3)
			local qoot = steps:GetCPSVector(4)
			local thingers = math.min(imcrazy,#moot)
			local wid = wodth/thingers
			local hodth = 0
			for i=1,#moot do 
				if moot[i] * 2 > hodth then
					hodth = moot[i] * 2
				end
			end
			hodth = hidth/hodth * 0.8
			for i=1,imcrazy do
				if i <= thingers then
					if moot[i] > 0 then
						self:GetChild(i):x(frameX + i * wid):zoomto(wid,moot[i]*2*hodth)
						self:GetChild(i):visible(true)
					else
						self:GetChild(i):visible(false)
					end

					if joot[i] > 0 then
						self:GetChild(i.."j"):x(frameX + i * wid):zoomto(wid,joot[i]*2*2*hodth)
						self:GetChild(i.."j"):visible(true)
					else
						self:GetChild(i.."j"):visible(false)
					end

					if hoot[i] > 0 then
						self:GetChild(i.."h"):x(frameX + i * wid):zoomto(wid,hoot[i]*2*3*hodth)
						self:GetChild(i.."h"):visible(true)
					else
						self:GetChild(i.."h"):visible(false)
					end

					if qoot[i] > 0 then
						self:GetChild(i.."q"):x(frameX + i * wid):zoomto(wid,qoot[i]*2*4*hodth)
						self:GetChild(i.."q"):visible(true)
					else
						self:GetChild(i.."q"):visible(false)
					end
				else
					self:GetChild(i):visible(false)
					self:GetChild(i.."j"):visible(false)
					self:GetChild(i.."h"):visible(false)
					self:GetChild(i.."q"):visible(false)
				end
			end
		end
	end,
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, 240):zoomto(wodth, hidth + 2):diffusealpha(1):valign(1):diffuse(color("1,1,1")):halign(0)
		end,
	}
}

local function makeaquad(i)
	local o = Def.Quad {
		Name = i,
		InitCommand = function(self)
			self:xy(frameX + i * 20, 240):zoomto(20, 0):diffusealpha(0.75):valign(1):diffuse(color(".75,.75,.75")):halign(1)
		end,
	}
	return o
end

local function makeaquadforjumpcounts(i)
	local o = Def.Quad {
		Name = i.."j",
		InitCommand = function(self)
			self:xy(frameX + i * 20, 240):zoomto(20, 0):diffusealpha(0.85):valign(1):diffuse(color(".5,.5,.5")):halign(1)
		end,
	}
	return o
end

local function makeaquadforhandcounts(i)
	local o = Def.Quad {
		Name = i.."h",
		InitCommand = function(self)
			self:xy(frameX + i * 20, 240):zoomto(20, 0):diffusealpha(0.95):valign(1):diffuse(color(".25,.25,.25")):halign(1)
		end,
	}
	return o
end

local function makeaquadforquadcounts(i)
	local o = Def.Quad {
		Name = i.."q",
		InitCommand = function(self)
			self:xy(frameX + i * 20, 240):zoomto(20, 0):diffusealpha(1):valign(1):diffuse(color(".1,.1,.1")):halign(1)
		end,
	}
	return o
end

for i=1,imcrazy do
	toot[#toot + 1] = makeaquad(i)
end
for i=1,imcrazy do
	toot[#toot + 1] = makeaquadforjumpcounts(i)
end
for i=1,imcrazy do
	toot[#toot + 1] = makeaquadforhandcounts(i)
end
for i=1,imcrazy do
	toot[#toot + 1] = makeaquadforquadcounts(i)
end

t[#t + 1] = toot


-- preview stuff should all be under a single actor frame if possible and not a child of msd where it inherits commands and stuff 
-- though this will break highlight since that's in the parent -mina
local function UpdatePreviewPos(self)
	if noteField then
		local pos = SCREENMAN:GetTopScreen():GetPreviewNoteFieldMusicPosition() / musicratio
		self:GetChild("Pos"):zoomto(math.min(pos,wodth), hidth)
	end
end
t[#t+1] = Def.ActorFrame {
	Name = "SeekBar",
	InitCommand = function(self)
		self:xy(50, 50)	-- eyeballed, not centered properly, this should all be moved out of the msd actor frame for sane control anyway -mina
		self:SetUpdateFunction(UpdatePreviewPos)
		self:visible(false)
	end,
	RefreshChartInfoMessageCommand = function(self)
		if GAMESTATE:GetCurrentSong() then
			musicratio = GAMESTATE:GetCurrentSong():MusicLengthSeconds() / wodth
		else
			MESSAGEMAN:Broadcast("DeletePreviewNoteField") -- kills it if we hit a pack... could be annoying but its the easiest way to hide stuff -mina
		end
	end,
	NoteFieldVisibleMessageCommand = function(self)
		self:visible(true)
	end,
	DeletePreviewNoteFieldMessageCommand = function(self)
		self:visible(false)
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:zoomto(wodth, hidth):diffuse(color(".2,.2,.2,.2")):diffusealpha(0.1):halign(0)	-- stuff like width should be var'd -mina
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
			self:zoomto(0, hidth):diffuse(color(".6,.4,.7")):diffusealpha(0.35):halign(0) -- this should really be behind the cd graph
		end
	},
	Def.Quad {
		Name = "Seek",
		InitCommand = function(self)
			self:zoomto(4, hidth):diffuse(color("1,.2,.5")):diffusealpha(1):halign(0.5)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				SCREENMAN:GetTopScreen():SetPreviewNoteFieldMusicPosition(	self:GetX() * musicratio  )
			end
		end
	},
}

return t
