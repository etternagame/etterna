return Def.ActorFrame{
	Def.Quad{
		InitCommand=cmd(xy,2,-2;zoomto,4,38),
		SetGradeCommand=function(self,params)
			if params.Grade then
				self:diffuse(getDifficultyColor("Difficulty_"..params.Difficulty))
				self:diffusealpha(0.5)
			else
				self:diffusealpha(0)
			end
		end
	},
	LoadFont("Common Normal") .. {
        InitCommand=cmd(xy,16,-1;zoom,0.5;maxwidth,WideScale(get43size(20),20)/0.5),
        SetGradeCommand=function(self,params)
			local sGrade = params.Grade or 'Grade_None'
			self:valign(0.5)
			self:settext(THEME:GetString("Grade",ToEnumShortString(sGrade)) or "")
			self:diffuse(getGradeColor(sGrade))
        end
	}
}