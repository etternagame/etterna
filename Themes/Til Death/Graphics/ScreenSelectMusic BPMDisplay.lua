return Def.BPMDisplay {
	Name="BPMDisplay";
	InitCommand=cmd(horizalign,left;zoom,0.50;);
	SetCommand=function(self)
    self:SetFromGameState()
    if getTabIndex() == 0 then 
      self:diffusealpha(1); 
    else 
      self:diffusealpha(0); 
    end; 
	end;
	CurrentSongChangedMessageCommand=cmd(playcommand,"Set");
	CurrentSongChangedMessageCommand=cmd(playcommand,"Set");
	CurrentCourseChangedMessageCommand=cmd(playcommand,"Set");
};