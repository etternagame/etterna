local t = Def.ActorFrame{}

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,SCREEN_CENTER_X-28,SCREEN_TOP+223;zoomto,60,20;halign,0;valign,0;diffuse,color("#999999CC"););
};
t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,SCREEN_CENTER_X-90,SCREEN_TOP+243;zoomto,100,150;halign,0;valign,0;diffuse,color("#333333CC"););
};

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,SCREEN_CENTER_X+5,SCREEN_TOP+213;zoom,0.50;halign,1;);
	BeginCommand=cmd(queuecommand,"Set");
	SetCommand=function(self) 
		self:settext("Online Scores")
	end;
};

local tapnames = {'Marvelous','Perfect','Great','Good','Bad','Miss','Combo'}
local judges = {'TapNoteScore_W1','TapNoteScore_W2','TapNoteScore_W3','TapNoteScore_W4','TapNoteScore_W5','TapNoteScore_Miss','TapNoteScore_W2'}

for k,v in ipairs(tapnames) do

	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(xy,SCREEN_CENTER_X-90,SCREEN_TOP+230+k*21;zoom,0.45;halign,0;settext,v;diffuse,byJudgment(judges[k]);diffusealpha,0.5);
		
	};
end


t[#t+1] = LoadActor("ScreenEvaluation decorations/default");

return t;
