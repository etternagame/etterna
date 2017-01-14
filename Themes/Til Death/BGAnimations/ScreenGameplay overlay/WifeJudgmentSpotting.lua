--[[ 
	Basically rewriting the c++ code to not be total shit so this can also not be total shit.
]]
	
local jcKeys = tableKeys(colorConfig:get_data().judgment)
local jcT = {}										-- A "T" following a variable name will designate an object of type table.

for i=1, #jcKeys do
	jcT[jcKeys[i]] = byJudgment(jcKeys[i])
end

local jdgT = {										-- Table of judgments for the judgecounter 
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo",
}

local dvCur																	
local jdgCur																-- Note: only for judgments with OFFSETS, might reorganize a bit later
local positive = getMainColor("positive")
local highlight = getMainColor("highlight")
local negative = getMainColor('negative')

-- We can also pull in some localized aliases for workhorse functions for a modest speed increase
local Round = notShit.round
local Floor = notShit.floor
local diffusealpha = Actor.diffusealpha
local diffuse = Actor.diffuse
local finishtweening = Actor.finishtweening
local linear = Actor.linear
local x = Actor.x
local queuecommand = Actor.queuecommand
local playcommand = Actor.queuecommand
local settext = BitmapText.settext
local Broadcast = MessageManager.Broadcast


-- Screenwide params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
isCentered = PREFSMAN:GetPreference("Center1Player")
local CenterX = SCREEN_CENTER_X
local mpOffset = 0
if not isCentered then
	CenterX = THEME:GetMetric("ScreenGameplay",string.format("PlayerP1%sX",ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStyleType())))
	mpOffset = SCREEN_CENTER_X
end
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								     **Wife deviance tracker. Basically half the point of the theme.**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	For every doot there is an equal and opposite scoot.
]]

local t = Def.ActorFrame{										
	Name = "WifePerch",					 
	JudgmentMessageCommand=function(self, msg)
		if msg.Offset ~= nil then
			dvCur = msg.Offset 
			jdgCur = msg.Judgment
			Broadcast(MESSAGEMAN, "SpottedOffset")
		end
	end,
}

-- Stuff you probably shouldn't turn off, music rate string display
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_BOTTOM-10;zoom,0.35;settext,getCurRateDisplayString())}



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
																	**LaneCover**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Old scwh lanecover back for now. Equivalent to "screencutting" on ffr; essentially hides notes for a fixed distance before they appear
on screen so you can adjust the time arrows display on screen without modifying their spacing from each other. 
]]	
	
t[#t+1] = LoadActor("lanecover")


	
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    	**Player Target Differential: Ghost target rewrite, average score gone for now**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Point differential to AA.
]]

-- Mostly clientside now. We set our desired target goal and listen to the results rather than calculating ourselves.
local target = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetGoal
GAMESTATE:GetPlayerState(PLAYER_1):SetTargetGoal(target/100)
 
d = Def.ActorFrame{	
	Def.Quad{InitCommand=cmd(xy,60 + mpOffset,(SCREEN_HEIGHT*0.62)-90;zoomto,60,16;diffuse,color("0,0,0,0.4");horizalign,left;vertalign,top)},
	-- Displays your current percentage score
	LoadFont("Common Large")..{											
		Name = "DisplayPercent",
		InitCommand=cmd(xy,115 + mpOffset,220;zoom,0.3;halign,1;valign,1),
		OnCommand=function(self)
			self:settextf("%05.2f%%", 0)
		end,
		JudgmentMessageCommand=function(self,msg)
			self:settextf("%05.2f%%", Floor(msg.WifePercent*100)/100)
		end
	},
}

-- We can save space by wrapping the personal best and set percent trackers into one function, however
-- this would make the actor needlessly cumbersome and unnecessarily punish those who don't use the
-- personal best tracker (although everything is efficient enough now it probably wouldn't matter)
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTrackerMode == 0 then
	d[#d+1] = LoadFont("Common Normal")..{											
		Name = "Set Percent Differential",
		InitCommand=cmd(xy,CenterX+26,SCREEN_CENTER_Y+30;zoom,0.4;halign,0;valign,1),
		JudgmentMessageCommand=function(self,msg)
			local tDiff = msg.WifeDifferential
			if tDiff >= 0 then 											
				diffuse(self,positive)
			else
				diffuse(self,negative)
			end
			self:settextf("%5.2f (%5.2f%%)", tDiff, target)
		end
	}
	else
	d[#d+1] = LoadFont("Common Normal")..{											
		Name = "PB Differential",
		InitCommand=cmd(xy,CenterX+26,SCREEN_CENTER_Y+30;zoom,0.4;halign,0;valign,1),
		JudgmentMessageCommand=function(self,msg)
			local tDiff = msg.WifePBDifferential
			if tDiff then
				local pbtarget = msg.WifePBGoal
				if tDiff >= 0 then
					diffuse(self,color("#00ff00"))
				else
					diffuse(self,negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, pbtarget*100)
			else
				tDiff = msg.WifeDifferential
				if tDiff >= 0 then 											
					diffuse(self,positive)
				else
					diffuse(self,negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, target)
			end
		end
	}
end



if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTracker then
	t[#t+1] = d
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											    	**Player judgment counter (aka pa counter)**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Counts judgments.
--]]

-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local frameX = 60 + mpOffset						 -- X position of the frame
local frameY = (SCREEN_HEIGHT*0.62)-90 				 -- Y Position of the frame
local spacing = 10									 -- Spacing between the judgetypes
local frameWidth = 60								 -- Width of the Frame
local frameHeight = ((#jdgT-1)*spacing)	- 8			 -- Height of the Frame
local judgeFontSize = 0.40							 -- Font sizes for different text elements 
local countFontSize = 0.35
local gradeFontSize = 0.45
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local jdgCounts = {}								 -- Child references for the judge counter

local j = Def.ActorFrame{
	OnCommand=function(self)
		for i=1,#jdgT do
			jdgCounts[jdgT[i]] = self:GetChild(jdgT[i])
		end
	end,
	JudgmentMessageCommand=function(self, msg)
		if jdgCounts[msg.Judgment] then
			settext(jdgCounts[msg.Judgment],msg.Val)
		end
	end																		
}																					

 local function makeJudgeText(judge,index)		-- Makes text
 	return LoadFont("Common normal")..{
 		InitCommand=cmd(xy,frameX+5,frameY+7+(index*spacing);zoom,judgeFontSize;halign,0),
 		OnCommand=function(self)
 			settext(self,getShortJudgeStrings(judge))
 			diffuse(self,jcT[judge])
 		end
 	}
 end
 
 local function makeJudgeCount(judge,index)		-- Makes county things for taps....
 	return LoadFont("Common Normal")..{
 		Name = judge,
		InitCommand=cmd(xy,frameWidth+frameX-5,frameY+7+(index*spacing);zoom,countFontSize;horizalign,right;settext,0) 	}
 end


-- Background
j[#j+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY+16;zoomto,frameWidth,frameHeight+16;diffuse,color("0,0,0,0.4");horizalign,left;vertalign,top)}

-- Build judgeboard
for i=1,#jdgT do
	j[#j+1] = makeJudgeText(jdgT[i],i)
	j[#j+1] = makeJudgeCount(jdgT[i],i)
end

-- Now add the completed judgment table to the primary actor frame t if enabled
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter then
	t[#t+1] = j
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**Player ErrorBar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Visual display of deviance values. 
--]]

-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local barcount = 30 									-- Number of bars. Older bars will refresh if judgments/barDuration exceeds this value. You don't need more than 40.
local frameX = CenterX 									-- X Positon (Center of the bar)
local frameY = SCREEN_CENTER_Y + 53						-- Y Positon (Center of the bar)
local frameHeight = 10 									-- Height of the bar
local frameWidth = capWideScale(get43size(240),240) 	-- Width of the bar
local barWidth = 2										-- Width of the ticks.
local barDuration = 0.75 								-- Time duration in seconds before the ticks fade out. Doesn't need to be higher than 1. Maybe if you have 300 bars I guess.
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local wscale = frameWidth/180 							-- so we aren't calculating it over and over again
local currentbar = 1 									-- so we know which error bar we need to update
local ingots = {}										-- references to the error bars

-- Makes the error bars. They position themselves relative to the center of the screen based on your dv and diffuse to your judgement value before disappating or refreshing
-- Should eventually be handled by the game itself to optimize performance
function smeltErrorBar(index)
	return Def.Quad{
		Name = index,
		InitCommand=cmd(xy,frameX,frameY;zoomto,barWidth,frameHeight;diffusealpha,0),
		UpdateErrorBarCommand=function(self)						-- probably a more efficient way to achieve this effect, should test stuff later
			finishtweening(self)									-- note: it really looks like shit without the fade out 
			diffusealpha(self,1)
			diffuse(self,jcT[jdgCur])
			x(self,frameX+dvCur*wscale)				
			linear(self,barDuration)
			diffusealpha(self,0)
		end
	}
end

local e = Def.ActorFrame{										
	InitCommand = function(self)
		for i=1,barcount do											-- basically the equivalent of using GetChildren() if it returned unnamed children numerically indexed
			ingots[#ingots+1] = self:GetChild(i)
		end
	end,
	SpottedOffsetMessageCommand=function(self)				
		currentbar = ((currentbar)%barcount) + 1
		playcommand(ingots[currentbar],"UpdateErrorBar")			-- Update the next bar in the queue
	end,
	DootCommand=function(self)
		self:RemoveChild("DestroyMe")
		self:RemoveChild("DestroyMe2")
	end,

	-- Centerpiece
	Def.Quad{InitCommand=cmd(diffuse,getMainColor('highlight');xy,frameX,frameY;zoomto,2,frameHeight)},
	
	-- Indicates which side is which (early/late) These should be destroyed after the song starts.
	LoadFont("Common Normal") .. {
		Name = "DestroyMe",
		InitCommand=cmd(xy,frameX+frameWidth/4,frameY;zoom,0.35),
		BeginCommand=cmd(settext,"Late";diffusealpha,0;smooth,0.5;diffusealpha,0.5;sleep,1.5;smooth,0.5;diffusealpha,0),
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe2",
		InitCommand=cmd(xy,frameX-frameWidth/4,frameY;zoom,0.35),
		BeginCommand=cmd(settext,"Early";diffusealpha,0;smooth,0.5;diffusealpha,0.5;sleep,1.5;smooth,0.5;diffusealpha,0;queuecommand,"Doot"),
		DootCommand=function(self)
			self:GetParent():queuecommand("Doot")
		end
	}
}

-- Initialize bars
for i=1,barcount do
	e[#e+1] = smeltErrorBar(i)
end

-- Add the completed errorbar frame to the primary actor frame t if enabled
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ErrorBar then
	t[#t+1] = e
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															   **Player Info**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Avatar and such, now you can turn it off. Planning to have player mods etc exported similarly to the nowplaying, and an avatar only option
]]
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PlayerInfo then
	t[#t+1] = LoadActor("playerinfo")
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														       **Full Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither. 
]]
 
-- User params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local posoffset = themeConfig:get_data().global.ProgressBar * (SCREEN_BOTTOM - 50)
local frameX = CenterX
local frameY = SCREEN_BOTTOM-30 - posoffset
local width = SCREEN_WIDTH/2-100
local height = 10
local alpha = 0.7
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

p = Def.ActorFrame{
	InitCommand=cmd(xy,frameX,frameY),
	Def.Quad{InitCommand=cmd(zoomto,width,height;diffuse,color("#666666");diffusealpha,alpha)},			-- background
	Def.SongMeterDisplay{
		InitCommand=function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth=width,
		Stream=Def.Quad{InitCommand=cmd(zoomy,height;diffuse,getMainColor("highlight"))}
	},
	LoadFont("Common Normal")..{																		-- title
		InitCommand=cmd(zoom,0.35;maxwidth,width*2),
		BeginCommand=cmd(settext,GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
	},
	LoadFont("Common Normal")..{																		-- total time
		InitCommand=cmd(x,width/2;zoom,0.35;maxwidth,width*2;halign,1),
		BeginCommand=function(self)
		local ttime = GetPlayableTime()
			settext(self,SecondsToMMSS(ttime))
			diffuse(self, ByMusicLength(ttime))
		end
	}
}

if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).FullProgressBar then
	t[#t+1] = p
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														      **Mini Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither. 
]]

local frameX = CenterX + 44
local frameY = SCREEN_CENTER_Y + 34
local width = 34
local height = 4
local alpha = 0.3

p = Def.ActorFrame{
	InitCommand=cmd(xy,frameX,frameY),
	Def.Quad{InitCommand=cmd(zoomto,width,height;diffuse,color("#666666");diffusealpha,alpha)}, 	-- background
	Def.Quad{InitCommand=cmd(x,1+width/2;zoomto,1,height;diffuse,color("#555555"))},				-- ending indicator
	Def.SongMeterDisplay{
		InitCommand=function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth=width,
		Stream=Def.Quad{InitCommand=cmd(zoomy,height;diffuse,getMainColor("highlight"))}
	}
}

if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).MiniProgressBar then
	t[#t+1] = p
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**BPM Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Better optimized frame update bpm display. 
]]

local BPM
local a = GAMESTATE:GetPlayerState(PLAYER_1):GetSongPosition()	
local r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
local GetBPS = SongPosition.GetCurBPS

local function UpdateBPM(self)
	local bpm = GetBPS(a) * r
	settext(BPM,Round(bpm,2))
end

t[#t+1] = Def.ActorFrame{
	InitCommand=function(self)
		BPM = self:GetChild("BPM")
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then			-- dont bother updating for single bpm files
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			settext(BPM,Round(GetBPS(a) * r,2))
		end
	end,
	LoadFont("Common Normal")..{
		Name="BPM",
		InitCommand=cmd(x,SCREEN_CENTER_X;y,SCREEN_BOTTOM-20;halign,0.5;zoom,0.40)
	}
}



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															**Combo Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	moving here eventually
]]



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														  **Judgment Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	moving here eventually
]]



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															 **NPS Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	re-enabling the old nps calc/graph for now 
]]

t[#t+1] = LoadActor("npscalc")



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															  **NPS graph**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ditto
]]



return t