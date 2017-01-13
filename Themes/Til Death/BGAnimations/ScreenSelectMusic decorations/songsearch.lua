local searchstring = ""
local englishes = {"a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",";"}
local numbershers = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local textsearchactive = false
local whee
local lastsearchstring = ""
local activeskillsetfilter = 0
local skillsetfilterqueries = {}

for i=1,#ms.SkillSets do 
	skillsetfilterqueries[i] = ""
end


local function searchInput(event)
	if event.type ~= "InputEventType_Release" and textsearchactive == true then
		if event.button == "Back" then
			searchstring = ""
			whee:SongSearch(searchstring)
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged")
			MESSAGEMAN:Broadcast("EndingSearch", {ActiveFilter = ""})
		elseif event.button == "Start" then
			resetTabIndex(0)
			--SetPersistentSearch(searchstring)
			MESSAGEMAN:Broadcast("EndingSearch", {ActiveFilter = searchstring})
			MESSAGEMAN:Broadcast("TabChanged")
		elseif event.DeviceInput.button == "DeviceButton_space" then					-- add space to the string
			searchstring = searchstring.." "
		elseif event.DeviceInput.button == "DeviceButton_backspace"then
			searchstring = searchstring:sub(1, -2)								-- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			searchstring = ""
		elseif event.DeviceInput.button == "DeviceButton_="  then
			searchstring = searchstring.."="
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
	elseif event.type ~= "InputEventType_Release" and activeskillsetfilter > 0 then
		if event.button == "Start" then
			textsearchactive = true
			activeskillsetfilter = 0
			MESSAGEMAN:Broadcast("NumericInputEnded")
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			skillsetfilterqueries[activeskillsetfilter] = skillsetfilterqueries[activeskillsetfilter]:sub(1, -2)								-- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			skillsetfilterqueries[activeskillsetfilter] = ""
		else
			for i=1,#numbershers do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..numbershers[i] then
					skillsetfilterqueries[activeskillsetfilter] = skillsetfilterqueries[activeskillsetfilter]..numbershers[i]
				end
			end
		end
		MESSAGEMAN:Broadcast("UpdateFilter")
		if skillsetfilterqueries[activeskillsetfilter] ~= "" then
			whee:SetSkillsetFilter(tonumber(skillsetfilterqueries[activeskillsetfilter]), activeskillsetfilter)
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
			textsearchactive = true
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
		else 
			self:visible(false)
			self:queuecommand("Off")
			textsearchactive = false
			SCREENMAN:set_input_redirected(PLAYER_1, false)
		end
	end,
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+250-capWideScale(get43size(120),30),frameY-90;zoom,0.7;halign,0.5;maxwidth,470),
		SetCommand=function(self) 
			if textsearchactive then
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





local i = 1

local f = Def.ActorFrame{
	InitCommand=cmd(xy,frameX+20,frameY-75;zoom,0.4;halign,0),
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-70;zoom,0.5;halign,0),
		SetCommand=cmd(settext, "Overall")
	},
	Def.Quad{
		InitCommand=cmd(zoomto,100,100),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				textsearchactive = false
				activeskillsetfilter = i
				MESSAGEMAN:Broadcast("NumericInputActive")
			end
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+170,frameY-70;zoom,0.5;halign,0),
		SetCommand=cmd(settext, skillsetfilterqueries[i]),
		UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
	},
}

t[#t+1] = f



return t