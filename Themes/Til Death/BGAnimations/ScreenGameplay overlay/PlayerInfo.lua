-- Various player and stage info, more text = fps drop so we should be sparing
local profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
local PlayerFrameX = 0
local PlayerFrameY = SCREEN_HEIGHT-50

local t = Def.ActorFrame{
	Def.Sprite {
		InitCommand=cmd(halign,0;valign,0;xy,PlayerFrameX,PlayerFrameY),
		BeginCommand=function(self)
			self:finishtweening()
			self:Load(THEME:GetPathG("","../"..getAvatarPath(PLAYER_1)))
			self:zoomto(50,50)
		end,
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(xy,PlayerFrameX+90,PlayerFrameY+24;halign,0;zoom,0.45;maxwidth,120;diffuse,getMainColor('positive')),
		SetCommand=function(self)
			self:settext(getDifficulty(GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty()))
			self:diffuse(getDifficultyColor(GetCustomDifficulty(GAMESTATE:GetCurrentSteps(PLAYER_1):GetStepsType(),GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty())))
		end,
		DoneLoadingNextSongMessageCommand=cmd(queuecommand,"Set")
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(xy,PlayerFrameX+52,PlayerFrameY+28;halign,0;zoom,0.75;maxwidth,50),
		SetCommand=function(self)
			local meter = GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(),1)
			self:settextf("%05.2f",meter)
			self:diffuse(ByMSD(meter))
		end,
		DoneLoadingNextSongMessageCommand=cmd(queuecommand,"Set")
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,PlayerFrameX+91,PlayerFrameY+39;halign,0;zoom,0.4;maxwidth,SCREEN_WIDTH*0.8),
		BeginCommand=cmd(settext, GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString('ModsLevel_Current'))
	},
	LoadFont("Common Normal")..{
		InitCommand=cmd(xy,PlayerFrameX+53,PlayerFrameY-2;halign,0;zoom,0.45),
		BeginCommand=cmd(settextf, "Judge: %d", GetTimingDifficulty()),
    },
	LoadFont("Common Normal")..{
		InitCommand=cmd(xy,PlayerFrameX+53,PlayerFrameY+8;halign,0;zoom,0.45),
		BeginCommand=cmd(settext, "Scoring: "..scoringToText(themeConfig:get_data().global.DefaultScoreType)),
    },
}
return t