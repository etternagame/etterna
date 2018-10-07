local t =
	Def.ActorFrame {
	-- splashy thing when you first start a song
	Name = "Splashy",
	DootCommand = function(self)
		self:RemoveAllChildren()
	end,
	Def.Quad {
		Name = "DestroyMe",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - 40):zoomto(400, 50):diffuse(getMainColor("highlight")):fadeleft(0.4):faderight(
				0.4
			):diffusealpha(0)
		end,
		OnCommand = function(self)
			self:smooth(0.5):diffusealpha(0.7):sleep(1):smooth(0.3):smooth(0.4):diffusealpha(0)
		end
	},
	LoadFont("Common Large") ..
		{
			Name = "DestroyMe2",
			InitCommand = function(self)
				self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - 50):zoom(0.5):diffusealpha(0):maxwidth(400 / 0.45)
			end,
			BeginCommand = function(self)
				self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
			end,
			OnCommand = function(self)
				self:smooth(0.5):diffusealpha(1):sleep(1):smooth(0.3):smooth(0.4):diffusealpha(0)
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "DestroyMe3",
			InitCommand = function(self)
				self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - 35):zoom(0.45):diffusealpha(0):maxwidth(400 / 0.45)
			end,
			BeginCommand = function(self)
				self:settext(GAMESTATE:GetCurrentSong():GetDisplaySubTitle())
			end,
			OnCommand = function(self)
				self:smooth(0.5):diffusealpha(1):sleep(1):smooth(0.3):smooth(0.4):diffusealpha(0)
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "DestroyMe4",
			InitCommand = function(self)
				self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - 25):zoom(0.45):diffusealpha(0)
			end,
			BeginCommand = function(self)
				self:settext(GAMESTATE:GetCurrentSong():GetDisplayArtist())
			end,
			OnCommand = function(self)
				self:smooth(0.5):diffusealpha(1):sleep(1):smooth(0.3):smooth(0.4):diffusealpha(0):queuecommand("Doot")
			end,
			DootCommand = function(self)
				self:GetParent():queuecommand("Doot")
			end
		}
}
return t
