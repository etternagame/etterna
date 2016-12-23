local grade = ...
assert(grade, "needs a grade")

-- todo: grade colors and such?
return LoadFont("Common normal")..{
	Text=THEME:GetString("Grade",grade);
	InitCommand=cmd(zoom,0.75;diffuse,color("#FFFFFF");shadowlength,0;strokecolor,color("#FFFFFF"));
};