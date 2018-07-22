local t = Def.ActorFrame {}
	-- Text
t[#t+1] = Def.ActorFrame {
	Def.Quad {
		InitCommand=function(self)
			self:zoomtowidth(SCREEN_WIDTH):zoomtoheight(30):horizalign(left):vertalign(top):y(SCREEN_TOP):diffuse(color("0,0,0,0"))
		end,
		OnCommand=function(self)
			self:finishtweening():diffusealpha(0.85)
		end,
		OffCommand=function(self)
			self:sleep(3):linear(0.5):diffusealpha(0)
		end
	},
	Def.BitmapText{
		Font="Common Normal",
		Name="Text",
		InitCommand=function(self)
			self:maxwidth(SCREEN_WIDTH*0.8):horizalign(left):vertalign(top):y(SCREEN_TOP+10):x(SCREEN_LEFT+10):diffusealpha(0)
		end,
		OnCommand=function(self)
			self:finishtweening():diffusealpha(1):zoom(0.5)
		end,
		OffCommand=function(self)
			self:sleep(3):linear(0.5):diffusealpha(0)
		end
	},
	SystemMessageMessageCommand = function(self, params)
		self:GetChild("Text"):settext( params.Message )
		self:playcommand( "On" )
		if params.NoAnimate then
			self:finishtweening()
		end
		self:playcommand( "Off" )
	end,
	HideSystemMessageMessageCommand = function(self)
		self:finishtweening()
	end
}

t[#t+1] = Def.ActorFrame {
	DFRStartedMessageCommand=function(self)
		self:visible(true)
	end,
	DFRFinishedMessageCommand=function(self,params)
		self:visible(false)
	end,
	BeginCommand=function(self)
		self:visible(false)
	end,
	Def.Quad {
		InitCommand=function(self)
			self:x(SCREEN_WIDTH/2):y(SCREEN_HEIGHT/2):zoomto(SCREEN_WIDTH/2, SCREEN_HEIGHT/2):diffuse(color("0.3,0.3,0.3,1"))
		end,
	},
	Def.BitmapText{
		Font="Common Normal",
		InitCommand=function(self)
			self:x(SCREEN_WIDTH/2):y(SCREEN_HEIGHT/2):diffusealpha(1):settext(""):maxwidth(SCREEN_WIDTH/2)
		end,
		DFRUpdateMessageCommand=function(self,params)
			self:settext(params.txt)
		end,
	}
}


return t
