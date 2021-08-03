-- mini progress bar. song progress but small

local width = 34 / GAMEPLAY_SIZING_RATIO
local height = 4 / GAMEPLAY_SIZING_RATIO
local alpha = 0.3

return Def.ActorFrame {
	Name = "MiniProgressBar",
	InitCommand = function(self)
		self:xy(MovableValues.MiniProgressBarX, MovableValues.MiniProgressBarY)
	end,
	
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:zoomto(width, height)
			self:diffuse(COLORS:getGameplayColor("miniProgressBarBG"))
			self:diffusealpha(alpha)
		end,
	},
	Def.Quad {
		Name = "SongEnd",
		InitCommand = function(self)
			self:x(1 + width / 2)
			self:zoomto(1, height)
			self:diffuse(COLORS:getGameplayColor("miniProgressBarEnd"))
			self:diffusealpha(1)
		end,
	},
	Def.SongMeterDisplay {
		Name = "Progress",
		InitCommand = function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth = width,
		Stream = Def.Quad {
			InitCommand = function(self)
				self:zoomy(height)
				self:diffuse(COLORS:getGameplayColor("miniProgressBar"))
				self:diffusealpha(1)
			end
		}
	}
}