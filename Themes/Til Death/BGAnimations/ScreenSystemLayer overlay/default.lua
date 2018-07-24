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

local www = 1366 * 0.8
local hhh = SCREEN_HEIGHT * 0.8
t[#t+1] = Def.ActorFrame {
	DFRStartedMessageCommand=function(self)
		self:visible(true)
	end,
	DFRFinishedMessageCommand=function(self,params)
		self:visible(false)
	end,
	BeginCommand=function(self)
		self:visible(false)
		self:x(www/8 + 10):y(SCREEN_BOTTOM - hhh/8 - 70)
	end,
	Def.Quad {
		InitCommand=function(self)
			self:zoomto(www/4, hhh/4):diffuse(color("0.1,0.1,0.1,0.8"))
		end,
	},
	Def.BitmapText{
		Font="Common Normal",
		InitCommand=function(self)
			self:diffusealpha(0.9):settext(""):maxwidth(www/4-40)
		end,
		DFRUpdateMessageCommand=function(self,params)
			self:settext(params.txt)
		end,
	}
}


return t
