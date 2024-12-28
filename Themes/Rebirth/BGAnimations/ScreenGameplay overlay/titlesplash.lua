local mods = {}

local translations = {
	InvalidMods = THEME:GetString("ScreenGameplay", "InvalidMods"),
	By = THEME:GetString("ScreenGameplay", "CreatedBy"),
}

local textSize = 0.8
local subtextSize = 0.75
local bigTextSize = 1.2
local authorSize = 0.60

local width = SCREEN_WIDTH / 3
local linesize = 75 / 1080 * SCREEN_HEIGHT
local height = linesize * 3

local t = Def.ActorFrame {
	Name = "Splashy",
	DootCommand = function(self)
		self:RemoveAllChildren()
	end,
	Def.Quad {
		Name = "DestroyMe",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
			self:zoomto(width, height)
			self:valign(1)
			self:diffuse(getGameplayColor("SplashBackground"))
			self:fadeleft(0.4):faderight(0.4)
			self:diffusealpha(0)
		end,
		OnCommand = function(self)
			self:smooth(0.5)
			self:diffusealpha(0.7)
			self:sleep(1)
			self:smooth(0.7)
			self:diffusealpha(0)
		end
	},
	LoadFont("Common Large") .. {
		Name = "DestroyMe2",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - linesize * 2)
			self:zoom(textSize)
			self:diffuse(getGameplayColor("SplashText"))
			self:diffusealpha(0)
			self:maxwidth(width / textSize)
		end,
		BeginCommand = function(self)
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
		end,
		OnCommand = function(self)
			self:smooth(0.5)
			self:diffusealpha(1)
			self:sleep(1)
			self:smooth(0.7)
			self:diffusealpha(0)
		end
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe3",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - linesize * 1.5)
			self:zoom(subtextSize)
			self:diffuse(getGameplayColor("SplashText"))
			self:diffusealpha(0)
			self:maxwidth(width / subtextSize)
		end,
		BeginCommand = function(self)
			self:settext(GAMESTATE:GetCurrentSong():GetDisplaySubTitle())
		end,
		OnCommand = function(self)
			self:smooth(0.5)
			self:diffusealpha(1)
			self:sleep(1)
			self:smooth(0.7)
			self:diffusealpha(0)
		end
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe4",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - linesize * 1.2)
			self:zoom(subtextSize)
			self:diffuse(getGameplayColor("SplashText"))
			self:diffusealpha(0)
			self:maxwidth(width / subtextSize)
		end,
		BeginCommand = function(self)
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayArtist())
		end,
		OnCommand = function(self)
			local time = 0.4
			if #mods > 0 then
				time = 2
			end
			self:smooth(0.5)
			self:diffusealpha(1)
			self:sleep(1)
			self:smooth(0.7)
			self:diffusealpha(0)
			self:sleep(time)
			self:queuecommand("Doot")
		end,
		DootCommand = function(self)
			self:GetParent():queuecommand("Doot")
		end
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe6",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y - linesize * 0.9)
			self:zoom(authorSize)
			self:diffuse(getGameplayColor("SplashText"))
			self:diffusealpha(0)
			self:maxwidth(width / subtextSize)
		end,
		BeginCommand = function(self)
			local auth = GAMESTATE:GetCurrentSong():GetOrTryAtLeastToGetSimfileAuthor()
			self:settextf("%s: %s", translations["By"], auth)
		end,
		OnCommand = function(self)
			self:smooth(0.5)
			self:diffusealpha(1)
			self:sleep(1)
			self:smooth(0.7)
			self:diffusealpha(0)
		end
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe5",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y + linesize)
			self:zoom(bigTextSize)
			self:diffuse(getGameplayColor("SplashText"))
			self:diffusealpha(0)
			self:valign(0)
		end,
		BeginCommand = function(self)
			mods = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():GetInvalidatingMods()
			local translated = {}
			if #mods > 0 then
				for _,mod in ipairs(mods) do
					table.insert(translated, THEME:HasString("OptionNames", mod) and THEME:GetString("OptionNames", mod) or mod)
				end
				self:settextf("%s\n%s", translations["InvalidMods"], table.concat(translated, "\n"))
			end
		end,
		OnCommand = function(self)
			self:smooth(0.5)
			self:diffusealpha(1)
			self:sleep(1)
			self:smooth(2.7)
			self:diffusealpha(0)
		end
	}
}
return t
