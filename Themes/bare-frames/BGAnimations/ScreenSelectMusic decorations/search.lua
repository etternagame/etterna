local t = Def.ActorFrame {}
-- Controls the song search relevant children of the ScreenSelectMusic decorations actorframe

local searchstring = ""
local active = false
local whee

-- imagine making a text input field just a regex char match of keyboard presses
local function searcher(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Back" then
			searchstring = ""
			whee:SongSearch(searchstring)
			MESSAGEMAN:Broadcast("EndingSearch")
		elseif event.button == "Start" then
			if not instantSearch then
				whee:SongSearch(searchstring)
			end
			MESSAGEMAN:Broadcast("EndingSearch")
		elseif event.DeviceInput.button == "DeviceButton_space" then -- add space to the string
			searchstring = searchstring .. " "
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			searchstring = searchstring:sub(1, -2) -- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete" then
			searchstring = ""
		elseif event.DeviceInput.button == "DeviceButton_=" then
			searchstring = searchstring .. "="
		else
			local CtrlPressed = INPUTFILTER:IsControlPressed()
			if event.DeviceInput.button == "DeviceButton_v" and CtrlPressed then
				searchstring = searchstring .. HOOKS:GetClipboard()
			elseif
			--if not nil and (not a number or (ctrl pressed and not online))
				event.char and event.char:match('[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%\'%"%>%<%?%/%~%|%w]') and
					(not tonumber(event.char) or CtrlPressed)
			 then
				searchstring = searchstring .. event.char
			end
		end
		if lastsearchstring ~= searchstring then
			MESSAGEMAN:Broadcast("UpdateString")
			if instantSearch then
				whee:SongSearch(searchstring)
			end
			lastsearchstring = searchstring
		end
    elseif event.type == "InputEventType_FirstPress" and active == false then
        if tonumber(event.char) == 4 then
            MESSAGEMAN:Broadcast("StartingSearch")
        end
    end
end

t[#t+1] = Def.ActorFrame {
    InitCommand = function(self)
        self:xy(SCREEN_CENTER_X + SCREEN_WIDTH/3, 20)
    end,
    OnCommand = function(self)
        SCREENMAN:GetTopScreen():AddInputCallback(searcher)
        whee = SCREENMAN:GetTopScreen():GetMusicWheel()
    end,
    EndingSearchMessageCommand = function(self)
        active = false
        self:playcommand("Set")
        SCREENMAN:set_input_redirected(PLAYER_1, false)
    end,
    StartingSearchMessageCommand = function(self)
        active = true
        self:playcommand("Set")
        SCREENMAN:set_input_redirected(PLAYER_1, true)
    end,

    LoadFont("Common Normal") .. {
        InitCommand = function(self)     
            self:zoom(0.4)
            self:settext("Song Search (4)")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "SearchInput",
        InitCommand = function(self)
            self:y(10)
            self:zoom(0.3)
            self:maxwidth(SCREEN_WIDTH/4 / 0.3)
        end,
        SetCommand = function(self)
            self:settext(searchstring)
        end,
        UpdateStringMessageCommand = function(self)
            self:playcommand("Set")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "SearchActive",
        InitCommand = function(self)
            self:y(20):zoom(.3):settext("ACTIVE"):visible(false)
        end,
        SetCommand = function(self)
            self:visible(active)
        end
    }
}


return t