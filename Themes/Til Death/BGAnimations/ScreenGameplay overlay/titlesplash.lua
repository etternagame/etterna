local mods = {}

local translated_info = {
	InvalidMods = THEME:GetString("ScreenGameplay", "InvalidMods")
}

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
				local time = 0.4
				if #mods > 0 then
					time = 2
				end
				self:smooth(0.5):diffusealpha(1):sleep(1):smooth(0.3):smooth(0.4):diffusealpha(0):sleep(time):queuecommand("Doot")
			end,
			DootCommand = function(self)
				self:GetParent():queuecommand("Doot")
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "DestroyMe5",
			InitCommand = function(self)
				self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - 10):zoom(0.7):diffusealpha(0):valign(0)
			end,
			BeginCommand = function(self)
				mods = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():GetInvalidatingMods()
				local translated = {}
				if #mods > 0 then
					for _,mod in ipairs(mods) do
						table.insert(translated, THEME:HasString("OptionNames", mod) and THEME:GetString("OptionNames", mod) or mod)
					end
					self:settextf("%s\n%s", translated_info["InvalidMods"], table.concat(translated, "\n"))
				end
			end,
			OnCommand = function(self)
				self:smooth(0.5):diffusealpha(1):sleep(1):smooth(0.3):smooth(0.4):smooth(2):diffusealpha(0)
			end
		}
}
return t
