local searchstring = ""
local englishes = {"a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local active = false
local whee

local function searchInput(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Back" then
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged")
		elseif event.button == "Start" then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			active = false
		elseif event.DeviceInput.button == "DeviceButton_space" then					-- add space to the string
			searchstring = searchstring.." "
		elseif event.DeviceInput.button == "DeviceButton_backspace"then
			searchstring = searchstring:sub(1, -2)								-- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			searchstring = ""
		else
			for i=1,#englishes do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..englishes[i] then
					searchstring = searchstring..englishes[i]
				end
			end
		end
		MESSAGEMAN:Broadcast("UpdateString")
		whee:SongSearch(searchstring)
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
			self:visible(true)
			active = true
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
			searchstring = ""
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
			self:settext("Start releases input redirect.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-175;zoom,0.4;halign,0),
		SetCommand=function(self) 
			self:settext("Delete resets search query.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},	
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-150;zoom,0.4;halign,0),
		SetCommand=function(self) 
			self:settext("Back returns to general tab.")
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