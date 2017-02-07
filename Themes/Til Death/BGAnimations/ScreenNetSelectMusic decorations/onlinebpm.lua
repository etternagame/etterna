--Merge two files "somethingelseBPMLabel" and "netplayBPMDisplay" into one file. -Misterkister

local bpmx = SCREEN_CENTER_X+10
local bpmy = SCREEN_CENTER_Y+35
local displayx = SCREEN_CENTER_X+45
local displayy = SCREEN_CENTER_Y+35

if round(GetScreenAspectRatio(),5) == 1.25 then

bpmx = SCREEN_CENTER_X
displayx = SCREEN_CENTER_X+35

end

local t = Def.ActorFrame{}

t[#t+1] = LoadFont("Common Normal") .. {
		Text="BPM";
		InitCommand=cmd(x,bpmx;y,bpmy;;zoom,0.50;);
	};
	
t[#t+1] = Def.BPMDisplay{
	File=THEME:GetPathF("BPMDisplay", "bpm");
	Name="BPMDisplay";
	InitCommand=cmd(x,displayx;y,displayy;zoom,0.50;);
	SetCommand=function(self) self:SetFromGameState() end;
	CurrentSongChangedMessageCommand=cmd(playcommand,"Set");
	CurrentCourseChangedMessageCommand=cmd(playcommand,"Set");
};

return t


