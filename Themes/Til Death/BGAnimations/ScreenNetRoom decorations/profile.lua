local update = false
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == (NSMAN:IsETTP() and 1 or 2) then
			self:queuecommand("On")
			self:visible(true)
			update = true
		else
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	PlayerJoinedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

local translated_info = {
	Validated = THEME:GetString("TabProfile", "ScoreValidated"),
	Invalidated = THEME:GetString("TabProfile", "ScoreInvalidated"),
	Online = THEME:GetString("TabProfile", "Online"),
	Local = THEME:GetString("TabProfile", "Local"),
	Title = THEME:GetString("TabProfile", "Title"),
	NextPage = THEME:GetString("TabProfile", "NextPage"),
	PreviousPage = THEME:GetString("TabProfile", "PreviousPage"),
	SaveProfile = THEME:GetString("TabProfile", "SaveProfile"),
	AssetSettings = THEME:GetString("TabProfile", "AssetSettingEntry"),
	ValidateAll = THEME:GetString("TabProfile", "ValidateAllScores")
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(360, 400)
local frameHeight = 350
local fontScale = 0.25
local scoresperpage = 25
local scoreYspacing = 10
local distY = 15
local offsetX = -10
local offsetY = 20
local rankingSkillset = 1
local rankingPage = 1
local numrankingpages = 10
local rankingWidth = frameWidth - capWideScale(15, 50)
local rankingX = capWideScale(30, 50)
local rankingY = capWideScale(40, 40)
local rankingTitleSpacing = (rankingWidth / (#ms.SkillSets))
local buttondiffuse = 0
local whee
local profile

if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
end

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
	end
}
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, offsetY):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(
			0.5
		)
	end
}
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 5, frameY + offsetY - 9):zoom(0.6):halign(0):diffuse(getMainColor("positive"))
			self:settext(translated_info["Title"])
		end
	}

local function byValidity(valid)
	if valid then
		return getMainColor("positive")
	end
	return byJudgment("TapNoteScore_Miss")
end

local function BroadcastIfActive(msg)
	if update then
		MESSAGEMAN:Broadcast(msg)
	end
end

local function ButtonActive(self)
	return isOver(self) and update
end

-- The input callback for mouse clicks already exists within the tabmanager and redefining it within the local scope does nothing but create confusion - mina
local r =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX, frameY)
	end,
	OnCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
	end
}

local function rankingLabel(i)
	local ths  -- the top highscore object - mina
	local ck
	local thssteps
	local thssong
	local xoffset

	local t =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(rankingX + offsetX, rankingY + offsetY + 10 + (i - 1) * scoreYspacing)
			self:RunCommandsOnChildren(
				function(self)
					self:halign(0):zoom(fontScale)
				end
			)
			self:visible(false)
		end,
		UpdateRankingMessageCommand = function(self)
			if rankingSkillset > 1 and update then
				ths = SCOREMAN:GetTopSSRHighScore(i + (scoresperpage * (rankingPage - 1)), ms.SkillSets[rankingSkillset])
				if ths then
					self:visible(true)
					ck = ths:GetChartKey()
					thssong = SONGMAN:GetSongByChartKey(ck)
					thssteps = SONGMAN:GetStepsByChartKey(ck)

					self:RunCommandsOnChildren(
						function(self)
							self:queuecommand("Display")
						end
					)
				end
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:maxwidth(100)
				end,
				DisplayCommand = function(self)
					self:halign(0.5)
					self:settext(((rankingPage - 1) * 25) + i .. ".")
					self:diffuse(byValidity(ths:GetEtternaValid()))
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:x(15):maxwidth(160)
				end,
				DisplayCommand = function(self)
					self:settextf("%5.2f", ths:GetSkillsetSSR(ms.SkillSets[rankingSkillset]))
					self:diffuse(byValidity(ths:GetEtternaValid()))
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:x(55):maxwidth(580)
				end,
				DisplayCommand = function(self)
					self:settext(thssong:GetDisplayMainTitle())
					self:diffuse(byValidity(ths:GetEtternaValid()))
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:x(220)
				end,
				DisplayCommand = function(self)
					self:halign(0.5)
					local ratestring = string.format("%.2f", ths:GetMusicRate()):gsub("%.?0+$", "") .. "x"
					self:settext(ratestring)
					self:diffuse(byValidity(ths:GetEtternaValid()))
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:x(240):maxwidth(160)
				end,
				DisplayCommand = function(self)
					self:settextf("%5.2f%%", ths:GetWifeScore() * 100)
					if not ths:GetEtternaValid() then
						self:diffuse(byJudgment("TapNoteScore_Miss"))
					else
						self:diffuse(getGradeColor(ths:GetWifeGrade()))
					end
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:x(300)
				end,
				DisplayCommand = function(self)
					local diff = thssteps:GetDifficulty()
					self:halign(0.5)
					self:diffuse(byDifficulty(diff))
					self:settext(getShortDifficulty(diff))
				end
			},
		Def.Quad {
			InitCommand = function(self)
				self:diffusealpha(buttondiffuse)
			end,
			DisplayCommand = function(self) -- hacky
				self:visible(true)
				self:zoomto(300, scoreYspacing)
			end,
			MouseRightClickMessageCommand = function(self)
				if ths and ButtonActive(self) then
					ths:ToggleEtternaValidation()
					BroadcastIfActive("UpdateRanking")
					if ths:GetEtternaValid() then
						ms.ok(translated_info["Validated"])
					else
						ms.ok(translated_info["Invalidated"])
					end
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				if ths and ButtonActive(self) then
					whee:SelectSong(thssong)
				end
			end
		}
	}
	return t
end

local function rankingButton(i)
	local t =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(rankingX + (i - 1) * rankingTitleSpacing, rankingY)
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(rankingTitleSpacing, 30):diffuse(getMainColor("frames")):diffusealpha(0.35)
			end,
			SetCommand = function(self)
				if i == rankingSkillset then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.35)
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				if ButtonActive(self) then
					rankingSkillset = i
					rankingPage = 1
					SCOREMAN:SortSSRs(ms.SkillSets[rankingSkillset])
					BroadcastIfActive("UpdateRanking")
				end
			end,
			UpdateRankingMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:diffuse(getMainColor("positive")):maxwidth(rankingTitleSpacing):maxheight(25):zoom(0.85)
				end,
				BeginCommand = function(self)
					self:settext(ms.SkillSetsTranslated[i])
				end
			}
	}
	return t
end

-- prev/next page
r[#r + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(10, frameHeight - offsetY):visible(false)
	end,
	UpdateRankingMessageCommand = function(self)
		if rankingSkillset > 1 then
			self:visible(true)
			self:RunCommandsOnChildren(
				function(self)
					self:queuecommand("Display")
				end
			)
		else
			self:visible(false)
		end
	end,
	Def.Quad {
		InitCommand = function(self)
			self:xy(300, -8):zoomto(40, 20):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				if rankingPage < numrankingpages then
					rankingPage = rankingPage + 1
				else
					rankingPage = 1
				end
				BroadcastIfActive("UpdateRanking")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:x(300):halign(0):zoom(0.3):diffuse(getMainColor("positive"))
				self:settext(translated_info["NextPage"])
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:y(-8):zoomto(65, 20):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				if rankingPage > 1 then
					rankingPage = rankingPage - 1
				else
					rankingPage = numrankingpages
				end
				BroadcastIfActive("UpdateRanking")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:halign(0):zoom(0.3):diffuse(getMainColor("positive"))
				self:settext(translated_info["PreviousPage"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:x(175):halign(0.5):zoom(0.3):diffuse(getMainColor("positive"))
			end,
			DisplayCommand = function(self)
				self:settextf("%i-%i", ((rankingPage - 1) * 25) + 1, rankingPage * 25)
			end
		}
}

for i = 1, scoresperpage do
	r[#r + 1] = rankingLabel(i)
end

-- Technically the "overall" skillset is used for single value display during music select/eval and isn't factored in to the profile rating
-- Only the specific skillsets are, and so overall should be used to display the specific skillset breakdowns separately - mina
for i = 1, #ms.SkillSets do
	r[#r + 1] = rankingButton(i)
end

local function littlebits(i)
	local t =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(frameX + 30, frameY + 50)
		end,
		UpdateRankingMessageCommand = function(self)
			if rankingSkillset == 1 and update then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:y(22 * i):halign(0):zoom(0.5):diffuse(getMainColor("positive"))
				end,
				SetCommand = function(self)
					self:settext(ms.SkillSetsTranslated[i] .. ":")
				end
			},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(240, 22 * i):halign(1):zoom(0.5)
				end,
				SetCommand = function(self)
					local rating = profile:GetPlayerSkillsetRating(ms.SkillSets[i])
					self:settextf("%5.2f", rating)
					self:diffuse(byMSD(rating))
				end
			}
	}
	return t
end

for i = 2, #ms.SkillSets do
	r[#r + 1] = littlebits(i)
end

local profilebuttons =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX + 60, frameHeight + 20)
	end,
	UpdateRankingMessageCommand = function(self)
		if rankingSkillset == 1 and update then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:diffuse(getMainColor("positive")):settext(translated_info["SaveProfile"]):zoom(0.3)
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(100, 20):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand = function(self)
			if ButtonActive(self) and rankingSkillset == 1 then
				if PROFILEMAN:SaveProfile(PLAYER_1) then
					ms.ok("Save successful")
				else
					ms.ok("Save failed")
				end
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:x(100):diffuse(getMainColor("positive")):settext(translated_info["AssetSettings"]):zoom(0.3)
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:x(100):zoomto(100, 20):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand = function(self)
			if ButtonActive(self) and rankingSkillset == 1 then
				SCREENMAN:SetNewScreen("ScreenAssetSettings")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:x(200):diffuse(getMainColor("positive")):settext(translated_info["ValidateAll"]):zoom(0.3)
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:x(200):zoomto(100, 20):diffusealpha(buttondiffuse)
		end,
		MouseLeftClickMessageCommand = function(self)
			if ButtonActive(self) and rankingSkillset == 1 then
				SCOREMAN:ValidateAllScores()
			end
		end
	}
}

t[#t + 1] = profilebuttons
t[#t + 1] = r
return t
