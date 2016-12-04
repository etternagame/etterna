--Help overlay

--Something relevant from the consensual thread heh...
--"10. If you leave it alone for a few seconds it pops up a screen with stupid-high amounts of unhelpful gibberish"

local enabled = false -- is the overlay currently enabled?
local show = themeConfig:get_data().global.HelpMenu -- whether to show after a certain amount of time as passed
local showTime = 30 --the "certain amount of time" from above in seconds
local curTime = GetTimeSinceStart() -- current time
local lastTime = GetTimeSinceStart() -- last input time

local function input(event)
	if event.type ~= "InputEventType_Release" then
		lastTime = GetTimeSinceStart()
		if event.DeviceInput.button == "DeviceButton_F12" then
			if not enabled then
				MESSAGEMAN:Broadcast("ShowHelpOverlay")
				enabled = true
			else
				MESSAGEMAN:Broadcast("HideHelpOverlay")
				enabled = false
			end;
		else
			MESSAGEMAN:Broadcast("HideHelpOverlay")
			enabled = false
		end;
	end;
	return false;
end

local function Update(self)
	if show then
		t.InitCommand=cmd(SetUpdateFunction,Update);
		curTime = GetTimeSinceStart()
		if (not enabled) and (curTime-lastTime > showTime) then
			MESSAGEMAN:Broadcast("ShowHelpOverlay")
			enabled = true
		end;
		--self:GetChild("Timer"):playcommand("Set")
	end;
end; 

local t = Def.ActorFrame{
	InitCommand=cmd(SetUpdateFunction,Update);
	OnCommand=function(self) self:diffusealpha(0) SCREENMAN:GetTopScreen():AddInputCallback(input) end;
	ShowHelpOverlayMessageCommand=cmd(stoptweening;smooth,0.3;diffusealpha,0.8);
	HideHelpOverlayMessageCommand=cmd(stoptweening;smooth,0.3;diffusealpha,0);
};

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,0,0;halign,0;valign,0;zoomto,SCREEN_WIDTH,SCREEN_HEIGHT;diffuse,color("#000000"););
};

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,0,35;halign,0;valign,1;zoomto,SCREEN_WIDTH/2,4;faderight,1;);
};

t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,5,32;halign,0;valign,1;zoom,0.55;settext,"Help Menu:";);
};
	
t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,5,SCREEN_HEIGHT-15;halign,0;valign,1;zoom,0.35;settext,"Press any key to hide this overlay.";);
};

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,5,SCREEN_HEIGHT-5;halign,0;valign,1;zoom,0.35;settext,"You can disable this overlay showing up automatically in Theme Options, but it can still be accessed by pressing F12.");
};

--have these strings in a separate file...?
local stringList = {
	{"Keys/Buttons","Functions"},
	{"1~5 or clicking the tabs","Switch to the corresponding tab. (e.g. 3=score, 4=profile, etc.)"},
	{"Doubletap <Select> or clicking the avatar","Open avatar switch overlay."},
	{"F12","Open help overlay."},
	{"<EffectUp>","While the Score tab is selected, select the previous saved score."},
	{"<EffectDown>","While the Score tab is selected, select the next saved score."},
	{"<EffectUp> while Holding <Select>","While the Score tab is selected, select the previous available rate."},
	{"<EffectDown> while Holding <Select>","While the Score tab is selected, select the next available rate."},
}

local function makeText(index)
	local t = Def.ActorFrame{}
	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(xy,5,50+(15*(index-1));zoom,0.4;halign,0;maxwidth,170/0.4);
		BeginCommand=cmd(queuecommand,"Set");
		SetCommand=function(self)
			self:settext(stringList[index][1])
		end;
		CodeMessageCommand=cmd(queuecommand,"Set");
	};
	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(xy,180,50+(15*(index-1));zoom,0.4;halign,0;);
		BeginCommand=cmd(queuecommand,"Set");
		SetCommand=function(self)
			self:settext(stringList[index][2])
		end;
		CodeMessageCommand=cmd(queuecommand,"Set");
	};
	return t
end;

--[[ --debug
t[#t+1] = LoadFont("Common Large")..{
	Name="Timer";
	InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_CENTER_Y+80;settext,"0.0");
	SetCommand=function(self)
		self:settextf("%0.2f %s",curTime-lastTime,tostring(curTime-showTime > lastTime))
	end;
};
--]]

for i=1,#stringList do
	t[#t+1] = makeText(i)
end;

return t