--Merge two files "somethingelseBPMLabel" and "netplayBPMDisplay" into one file. -Misterkister

local t = Def.ActorFrame{}

t[#t+1] = LoadFont("Common Normal") .. {
		Text="BPM";
		InitCommand=cmd(x,SCREEN_CENTER_X+10;y,SCREEN_CENTER_Y+35;;zoom,0.50;);
	};
	
t[#t+1] = Def.BPMDisplay{
	File=THEME:GetPathF("BPMDisplay", "bpm");
	Name="BPMDisplay";
	InitCommand=cmd(x,SCREEN_CENTER_X+45;y,SCREEN_CENTER_Y+35;zoom,0.50;);
	SetCommand=function(self) self:SetFromGameState() end;
	CurrentSongChangedMessageCommand=cmd(playcommand,"Set");
	CurrentCourseChangedMessageCommand=cmd(playcommand,"Set");
};

return t


