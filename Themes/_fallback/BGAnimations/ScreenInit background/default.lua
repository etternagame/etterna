local t = Def.ActorFrame {}

t[#t + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:Center()
	end,
	Def.Quad {
		InitCommand = function(self)
			self:scaletoclipped(SCREEN_WIDTH, SCREEN_HEIGHT)
		end,
		OnCommand = function(self)
			self:diffuse(color("0,0,0,1")):diffusetopedge(color("0.1,0.1,0.1,1"))
		end
	}
}
t[#t + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:Center()
	end,
	Def.ActorFrame {
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(SCREEN_WIDTH, 128)
			end,
			OnCommand = function(self)
				self:diffusealpha(1):sleep(1.5):linear(0.25)
			end
		},
		LoadActor("ssc") ..
			{
				OnCommand = function(self)
					self:diffusealpha(0):linear(1):diffusealpha(1):sleep(0.75):linear(0.25):diffusealpha(0)
				end
			}
	},
	Def.ActorFrame {
		OnCommand = function(self)
			self:playcommandonchildren("ChildrenOn")
		end,
		ChildrenOnCommand = function(self)
			self:diffusealpha(0):sleep(2):linear(0.25):diffusealpha(1)
		end,
		LoadFont("Common Normal") ..
			{
				Text = ProductID(),
				InitCommand = function(self)
					self:y(-20):zoom(0.75)
				end,
				OnCommand = function(self)
					self:diffuse(color("0,0,0,1"))
				end
			},
		LoadFont("Common Normal") ..
			{
				Text = THEME:GetThemeDisplayName(),
				OnCommand = function(self)
					self:diffuse(color("0,0,0,1"))
				end
			},
		LoadFont("Common Normal") ..
			{
				Text = "Created by " .. THEME:GetThemeAuthor(),
				InitCommand = function(self)
					self:y(24):zoom(0.75)
				end,
				OnCommand = function(self)
					self:diffuse(color("0,0,0,1"))
				end
			}
	}
}

return t
