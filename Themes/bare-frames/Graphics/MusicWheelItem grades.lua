-- Controls the grades that display on the Music Wheel
return Def.ActorFrame {
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(10, -1):zoom(0.3):maxwidth(WideScale(get43size(20), 20) / 0.5):halign(1)
		end,
		SetGradeCommand = function(self, params)
			local sGrade = params.Grade or "Grade_None"
			self:settext(THEME:GetString("Grade", ToEnumShortString(sGrade)) or "")
		end
	}
}
