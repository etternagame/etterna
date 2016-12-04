--file containing stuff for cursors.
--this should only be loaded by screen overlays, 
--otherwise the inputcallback function won't be able to find the actors.

local maxChild = 20
local curIndex = 0

local function input(event)
	local top = SCREENMAN:GetTopScreen():GetChildren().Overlay
	if event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			curIndex = (curIndex+1)%20
			MESSAGEMAN:Broadcast("Click")
		end;
	end;
return false;
end;

function cursorClick(index)
	return LoadActor(THEME:GetPathG("","_circle")) .. {
		Name="CursorClick";
		InitCommand=cmd(diffusealpha,0);
		ClickMessageCommand=function(self)
			if index == curIndex then
				self:finishtweening()
				self:xy(INPUTFILTER:GetMouseX(),INPUTFILTER:GetMouseY())
				self:diffusealpha(1)
				self:zoom(0)
				self:decelerate(0.5)
				self:diffusealpha(0)
				self:zoom(1)
			end
		end;
	}
end

local t = Def.ActorFrame{
	Name="Cursor";
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end;
}

for i=0,maxChild do
	t[#t+1] = cursorClick(i)
end

t[#t+1] = Def.Quad{
	Name="Cursor";
	InitCommand=cmd(xy,0,0;zoomto,4,4;rotationz,45;);
};

local function Update(self)
	t.InitCommand=cmd(SetUpdateFunction,Update);
    --self:GetChild("MouseXY"):settextf("X:%5.2f Y:%5.2f W:%5.2f",INPUTFILTER:GetMouseX(),INPUTFILTER:GetMouseY(),INPUTFILTER:GetMouseWheel())
    if not PREFSMAN:GetPreference("Windowed") then
   		self:GetChild("Cursor"):xy(INPUTFILTER:GetMouseX(),INPUTFILTER:GetMouseY())
   		self:GetChild("Cursor"):visible(true)
   	else
   		self:GetChild("Cursor"):visible(false)
   	end;
    --self:GetChild("FullScreen"):settextf("FullScreen: %s",tostring(not PREFSMAN:GetPreference("Windowed")))
end; 
t.InitCommand=cmd(SetUpdateFunction,Update);

return t