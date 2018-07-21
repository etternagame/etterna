-- if score is ever nil we done goofed way before this screen is ever loaded -mina
-- generalized code to reduce redundancy, load general code instead

local t = Def.ActorFrame {
	CodeMessageCommand=function(self,params)
		if params.Name == "PlotCancel" or params.Name == "PlotExit" or params.Name == "PlotThickens" or params.Name == "PlotTwist" or params.Name == "StarPlot64" or params.Name == "SheriffOfPlottingham" then
			SCREENMAN:GetTopScreen():Cancel()	-- need this so we can leave
		end
	end
}
t[#t+1] = LoadActor("offsetplot")
return t