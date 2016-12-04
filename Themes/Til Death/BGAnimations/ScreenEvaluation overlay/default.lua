local t = Def.ActorFrame{}
t[#t+1] = LoadActor("../_frame");
t[#t+1] = LoadActor("../_PlayerInfo");
t[#t+1] = LoadActor("currenttime");


--what the settext says
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,5,32;halign,0;valign,1;zoom,0.55;diffuse,getMainColor('positive');settext,"Results:";);
}

--Group folder name
local frameWidth = 280
local frameHeight = 20
local frameX = SCREEN_WIDTH-5
local frameY = 15

t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=cmd(xy,frameX,frameY+5;halign,1;zoom,0.55;maxwidth,(frameWidth-40)/0.35);
	BeginCommand=cmd(queuecommand,"Set";diffuse,getMainColor('positive'));
	SetCommand=function(self)
		local song = GAMESTATE:GetCurrentSong()
		local course = GAMESTATE:GetCurrentCourse()
		if song ~= nil and (not GAMESTATE:IsCourseMode()) then
			self:settext(song:GetGroupName())
		end;
		if course ~= nil and GAMESTATE:IsCourseMode() then
			self:settext(course:GetGroupName())
		end;
	end;
};

t[#t+1] = LoadActor("../_cursor");

return t