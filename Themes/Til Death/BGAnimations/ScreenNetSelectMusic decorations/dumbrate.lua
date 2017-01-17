--A dumb way of getting the rate to update online. -Misterkister

--Local vars
local update = false
local steps
local song
local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320,400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]
local greatest = 0
local steps
local meter = {}
local curateX = frameX+frameWidth+5
local curateY = frameY+offsetY+140
meter[1] = 0.00

--4:3 ratio. -Misterkister
if not IsUsingWideScreen() == true then
curateX = frameX+frameWidth-40
curateY = frameY+offsetY-20

end

--Actor Frame
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0),
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 0 then
			self:queuecommand("On")
			self:visible(true)
			song = GAMESTATE:GetCurrentSong()
			steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
			
			--Find max MSD value, store MSD values in meter[]
			greatest=0
			if song and steps then
				for i=1,#ms.SkillSets do 
					meter[i+1] = steps:GetMSD(getCurRateValue(), i)
					if meter[i+1] > meter[greatest+1] then
						greatest = i
					end
				end
			end
			
			MESSAGEMAN:Broadcast("UpdateMSDInfo")
			update = true
		else 
			self:queuecommand("Off")
			update = false
		end
	end,
	CurrentRateChangedMessageCommand=cmd(queuecommand,"Set"),
	RefreshChartInfoMessageCommand=cmd(queuecommand,"Set"),
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
}

--Skillset label function
local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+35,frameY+120 + 22*i;halign,0;valign,0;zoom,0.5;maxwidth,110/0.6),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				--skillset name
				if song and steps then
					self:settext(ms.SkillSets[i]..":")
				else
					self:settext("")
				end
				--highlight
				if greatest == i then
					self:diffuse(getMainColor('positive'))
				else
					self:diffuse(getMainColor('negative'))
				end
			end,
			UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,frameX+225,frameY+120 + 22*i;halign,1;valign,0;zoom,0.5;maxwidth,110/0.6),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				if song and steps then
					self:settextf("%05.2f",meter[i+1])
					self:diffuse(ByMSD(meter[i+1]))
				else
					self:settext("")
				end
			end,
			CurrentRateChangedMessageCommand=cmd(queuecommand,"Set"),
			UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
		}
	}
	return t
end

-- Music Rate Display
t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=cmd(xy,curateX,curateY;visible,true;halign,0;zoom,0.35;maxwidth,capWideScale(get43size(360),360)/capWideScale(get43size(0.45),0.45)),
	SetCommand=function(self)
		self:settext(getCurRateDisplayString())
	end,
	CurrentRateChangedCommand=cmd(queuecommand,"set")
}

return t
