return Def.BPMDisplay {
	File=THEME:GetPathF("BPMDisplay", "bpm");
	Name="BPMDisplay";
	InitCommand=cmd(x,SCREEN_CENTER_X-45;y,SCREEN_CENTER_Y-190;zoom,0.50;);
	SetCommand=function(self) self:SetFromGameState() end;
	CurrentSongChangedMessageCommand=cmd(playcommand,"Set");
	CurrentCourseChangedMessageCommand=cmd(playcommand,"Set");
};