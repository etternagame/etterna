local sButton = Var "Button"
local pn = Var "Player"

local Buttons = {
	["Dance_Single"] = {
		"Left",
		"Down",
		"Up",
		"Right",
	},
}

local getstyle = ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStepsType())

print(getstyle)

local t = Def.ActorFrame {}

if sButton == Buttons[getstyle][1] then
	fReceptor = {}
	for i = 1,#Buttons[getstyle] do
		t[#t+1] = NOTESKIN:LoadActor( Buttons[getstyle][i], "Overlay Receptor" )..{
			InitCommand=function(self) 
				self:x((i-1)*64) 
				fReceptor[Buttons[getstyle][i]] = self;
			end;
		}
	end 
end

return t;