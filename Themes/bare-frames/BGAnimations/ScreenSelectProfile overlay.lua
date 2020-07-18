-- Controls the topmost layer of ScreenSelectProfile
-- all of this basically allows for selecting a profile while still supporting players joining
-- but player joining is obsoleted so a lot of this code can be simplified

local translated_info = {
	Title = "Select Profile",
	SongPlayed = "Song Played",
	SongsPlayed = "Songs Played",
	NoProfile = "No Profile",
	PressStart = "Press Start"
}

function GetLocalProfiles()
	local t = {}

	for p = 0, PROFILEMAN:GetNumLocalProfiles() - 1 do
		local profileID = PROFILEMAN:GetLocalProfileIDFromIndex(p)
		local profile = PROFILEMAN:GetLocalProfileFromIndex(p)
		local ProfileCard =
			Def.ActorFrame {
				Name = p,
			LoadFont("Common Large") ..
				{
					Text = string.format("%s: %.2f", profile:GetDisplayName(), profile:GetPlayerRating()),
					InitCommand = function(self)
						self:y(-10):zoom(0.4):ztest(true, maxwidth, (200 - 34 - 4) / 0.4)	
					end
				},
			LoadFont("Common Normal") ..
				{
					InitCommand = function(self)
						self:y(8):zoom(0.5):vertspacing(-8):ztest(true):maxwidth((200 - 34 - 4) / 0.5)
					end,
					BeginCommand = function(self)
						local numSongsPlayed = profile:GetNumTotalSongsPlayed()
						local s = numSongsPlayed == 1 and translated_info["SongPlayed"] or translated_info["SongsPlayed"]
						self:settext(numSongsPlayed .. " " .. s)
					end
				}
		}
		t[#t + 1] = ProfileCard
	end

	return t
end

function LoadPlayerStuff(Player)
	local t = {}
	t[#t + 1] =
		Def.ActorFrame {
		Name = "SmallFrame",
		InitCommand = function(self)
			self:y(-2)
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(200, 40 + 2)
			end,
			OnCommand = function(self)
				self:diffusealpha(0.3)
			end
		}
	}

	t[#t + 1] =
		Def.ActorScroller {
		Name = "Scroller",
		NumItemsToDraw = 6,
		OnCommand = function(self)
			self:y(1):SetFastCatchup(true):SetMask(200, 58):SetSecondsPerItem(0.15)
		end,
		TransformFunction = function(self, offset, itemIndex, numItems)
			local focus = scale(math.abs(offset), 0, 2, 1, 0)
			self:visible(false)
			self:y(math.floor(offset * 40))
		end,
		children = GetLocalProfiles()
	}

	return t
end

function UpdateInternal3(self, Player)
	local pn = (Player == PLAYER_1) and 1
	local frame = self:GetChild(string.format("P%uFrame", pn))
	local scroller = frame:GetChild("Scroller")
	local smallframe = frame:GetChild("SmallFrame")

	if GAMESTATE:IsHumanPlayer(Player) then
		frame:visible(true)
			smallframe:visible(true)
			scroller:visible(true)
			local ind = SCREENMAN:GetTopScreen():GetProfileIndex(Player)
			if ind > 0 then
				scroller:SetDestinationItem(ind - 1)
			else
				if SCREENMAN:GetTopScreen():SetProfileIndex(Player, 1) then
					scroller:SetDestinationItem(0)
					self:queuecommand("UpdateInternal2")
				else
					smallframe:visible(false)
					scroller:visible(false)
				end
			end
	else
		scroller:visible(false)
		smallframe:visible(false)
	end
end

local theThingVeryImportant
local startSound

local function input(event)
    if event.type == "InputEventType_FirstPress" then
        if event.button == "Back" then
            SCREENMAN:GetTopScreen():Cancel()
        elseif event.button == "Start" then
            startSound:queuecommand("StartButton")
        elseif event.button == "Down" or event.button == "MenuDown" then
            local ind = SCREENMAN:GetTopScreen():GetProfileIndex(PLAYER_1)
            if ind > 0 then
                if SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, ind + 1) then
                    MESSAGEMAN:Broadcast("DirectionButton")
                    theThingVeryImportant:queuecommand("UpdateInternal2")
                end
            end
        elseif event.button == "Up" or event.button == "MenuUp" then
            local ind = SCREENMAN:GetTopScreen():GetProfileIndex(PLAYER_1)
            if ind > 1 then
                if SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, ind - 1) then
                    MESSAGEMAN:Broadcast("DirectionButton")
                    theThingVeryImportant:queuecommand("UpdateInternal2")
                end
            end
        end
	end
	return false
end

local t = Def.ActorFrame {}

t[#t + 1] =
	Def.ActorFrame {
    InitCommand = function(self)
        theThingVeryImportant = self
    end,
    OnCommand = function(self)
        SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, 0)
    end,
	OnCommand = function(self, params)
		self:queuecommand("UpdateInternal2")
	end,
	UpdateInternal2Command = function(self)
		UpdateInternal3(self, PLAYER_1)
	end,
	children = {
		Def.ActorFrame {
			Name = "P1Frame",
			InitCommand = function(self)
				self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y)
			end,
            OnCommand = function(self)
                SCREENMAN:GetTopScreen():AddInputCallback(input)
				self:zoom(0):bounceend(0.2):zoom(1)
			end,
			OffCommand = function(self)
				self:bouncebegin(0.2):zoom(0)
			end,
			PlayerJoinedMessageCommand = function(self, param)
				if param.Player == PLAYER_1 then
					self:zoom(1.15):bounceend(0.175):zoom(1.0)
				end
			end,
			children = LoadPlayerStuff(PLAYER_1)
		},
		-- sounds
		LoadActor(THEME:GetPathS("Common", "start")) ..
        {
            InitCommand = function(self)
                startSound = self
            end,
            StartButtonCommand = function(self)
                self:play()
                self:sleep(0.2)
                self:queuecommand("Done")
            end,
            DoneCommand = function(self)
                SCREENMAN:GetTopScreen():Finish()
            end
        },
		LoadActor(THEME:GetPathS("Common", "value")) ..
        {
            DirectionButtonMessageCommand = function(self)
                self:play()
            end
        }
	}
}

return t
