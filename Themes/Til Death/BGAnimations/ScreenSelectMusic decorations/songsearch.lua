local searchstring = ""
local englishes = {"?","-",".",",","1","2","3","4","5","6","7","8","9","0","a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",";"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local active = false
local whee
local lastsearchstring = ""
local CtrlPressed = false

local function searchInput(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Back" then
			searchstring = ""
			whee:SongSearch(searchstring)
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged")
			MESSAGEMAN:Broadcast("EndingSearch")
		elseif event.button == "Start" then
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("EndingSearch")
			MESSAGEMAN:Broadcast("TabChanged")
		elseif event.DeviceInput.button == "DeviceButton_space" then					-- add space to the string
			searchstring = searchstring.." "
		elseif event.DeviceInput.button == "DeviceButton_backspace"then
			searchstring = searchstring:sub(1, -2)								-- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			searchstring = ""
		elseif event.DeviceInput.button == "DeviceButton_="  then
			searchstring = searchstring.."="	
		elseif event.DeviceInput.button == "DeviceButton_v" and CtrlPressed then
			searchstring = searchstring..HOOKS:GetClipboard()
		else
			for i=1,#englishes do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..englishes[i] then
					searchstring = searchstring..englishes[i]
				end
			end
		end
		if lastsearchstring ~= searchstring then
			MESSAGEMAN:Broadcast("UpdateString")
			whee:SongSearch(searchstring)
			lastsearchstring = searchstring
		end
	end
	if event.DeviceInput.button == "DeviceButton_right ctrl" or event.DeviceInput.button == "DeviceButton_left ctrl" then
		if event.type == "InputEventType_Release" then
			CtrlPressed = false
		else
			CtrlPressed = true
		end
	end
end

local t = Def.ActorFrame{
	OnCommand=function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		SCREENMAN:GetTopScreen():AddInputCallback(searchInput)
		self:visible(false)
	end,
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 3 then
			ms.ok("Song search activated")
			MESSAGEMAN:Broadcast("BeginningSearch")
			self:visible(true)
			active = true
			whee:Move(0)
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
		else 
			self:visible(false)
			self:queuecommand("Off")
			active = false
			SCREENMAN:set_input_redirected(PLAYER_1, false)
		end
	end,
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+250-capWideScale(get43size(120),30),frameY-90;zoom,0.7;halign,0.5;maxwidth,470),
		SetCommand=function(self) 
			if active then
				self:settext("Search Active:")
				self:diffuse(getGradeColor("Grade_Tier03"))
			else
				self:settext("Search Complete:")
				self:diffuse(byJudgment("TapNoteScore_Miss"))
			end
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+250-capWideScale(get43size(120),30),frameY-50;zoom,0.7;halign,0.5;maxwidth,470),
		SetCommand=function(self) 
			self:settext(searchstring)
		end,
	UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-200;zoom,0.4;halign,0),
		SetCommand=function(self) 
			self:settext("Start to lock search results.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-175;zoom,0.4;halign,0),
		SetCommand=function(self) 
			self:settext("Back to cancel search.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-150;zoom,0.4;halign,0),
		SetCommand=function(self) 
			self:settext("Delete resets search query.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal")..{
		InitCommand=cmd(xy,frameX+20,frameY+70;zoom,0.5;halign,0),
		SetCommand=function(self) 
			self:settext("Currently supports standard english alphabet only.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
}

return t