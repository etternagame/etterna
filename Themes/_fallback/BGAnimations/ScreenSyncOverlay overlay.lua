local revert_sync_changes = THEME:GetString("ScreenSyncOverlay", "revert_sync_changes")
local change_song_offset = THEME:GetString("ScreenSyncOverlay", "change_song_offset")
local change_machine_offset = THEME:GetString("ScreenSyncOverlay", "change_machine_offset")
local hold_alt = THEME:GetString("ScreenSyncOverlay", "hold_alt")

local sh_r = THEME:GetMetric("Common", "ScreenHeight")/480
local showadj = true

local helptexts = {
	revert_sync_changes .. ":",
	"    F4",
	change_song_offset .. ":",
	"    F11/F12",
	change_machine_offset .. ":",
	"    Shift + F11/F12",
	hold_alt
}

return Def.ActorFrame {
	Def.Quad {
		Name = "quad",
		InitCommand = function(self)
			self:visible(false):diffuse(0,0,0,0):horizalign(right)
		end,
		ShowCommand = function(self)
			if GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() == true then
				self:vertalign(bottom):xy(_screen.w, _screen.h)
			else
				self:vertalign(top):xy(_screen.w, 0)
			end
			local help_text = self:GetParent():GetChild("help_text")
			self:zoomtowidth(help_text:GetZoomedWidth() + 20):zoomtoheight(help_text:GetZoomedHeight() + 20)
			self:visible(true)
			self:stoptweening():decelerate(.3):diffusealpha(.5):sleep(6):linear(.3):diffusealpha(0)
		end,
		HideCommand = function(self)
			self:finishtweening()
		end
	},
	Def.BitmapText {
		Name = "help_text",
		Font = "Common Normal",
		InitCommand = function(self)
			self:diffuse(1,1,1,0):horizalign(left):shadowlength(2):settext(table.concat(helptexts, "\n"))
			self:zoom(math.min(1,0.7*sh_r)):visible(false)
		end,
		ShowCommand = function(self)
			if GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() == true then
				self:vertalign(bottom):xy(_screen.w - self:GetZoomedWidth() - 10, _screen.h - 10)
			else
				self:vertalign(top):xy(_screen.w - self:GetZoomedWidth() - 10, 10)
			end
			self:visible(true)
			self:stoptweening():decelerate(.3):diffusealpha(1):sleep(6):linear(.3):diffusealpha(0)
		end,
		HideCommand = function(self)
			self:finishtweening()
			self:visible(false)
		end
	},
	LoadFont("Common Large") ..
	{
		Name = "Status",
		Font = "ScreenSyncOverlay status",
		InitCommand = function(self)
			ActorUtil.LoadAllCommands(self, "ScreenSyncOverlay")
			self:zoom(0.5):playcommand("On")
		end,
		SetStatusCommand = function(self, param)
			self:settext(param.text)
			if GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() == true then
				self:y(SCREEN_HEIGHT*0.15)
			else
				self:y(SCREEN_HEIGHT*0.85)
			end
		end,
		HideCommand = function(self)
			self:settext("")
		end
	},
	LoadFont("Common Large") ..
	{
		Name = "Adjustments",
		Font = "ScreenSyncOverlay adjustments",
		InitCommand = function(self)
			ActorUtil.LoadAllCommands(self, "ScreenSyncOverlay")
			self:playcommand("On")
		end,
		SetAdjustmentsCommand = function(self, param)
			self:visible(param.visible):settext(param.text)
			if param.visible and showadj then
				if SCREEN_WIDTH/SCREEN_HEIGHT < 5.02/4 then
					self:zoom(0.25*sh_r):bounceend(0.05):zoom(math.min(1,0.55*sh_r)/2)
				elseif SCREEN_WIDTH/SCREEN_HEIGHT < 4.02/3 then
					self:zoom(0.25*sh_r):bounceend(0.05):zoom(math.min(1,0.65*sh_r)/2)
				else
					self:zoom(0.25*sh_r):bounceend(0.05):zoom(math.min(1,0.85*sh_r)/2)
				end
				showadj = false
			elseif not param.visible then
				showadj = true
			end
		end,
		HideCommand = function(self)
			self:settext("")
		end
	}
}
