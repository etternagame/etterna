--Commenting out the Player 2 stuff so if someone is attempting to use this theme for versus or 2P side, it's not going to work. Go use Prim's original spawnhack theme for that. -Misterkister

function GetLocalProfiles()
	local t = {};

	for p = 0,PROFILEMAN:GetNumLocalProfiles()-1 do
		local profileID = PROFILEMAN:GetLocalProfileIDFromIndex(p)
		local profile=PROFILEMAN:GetLocalProfileFromIndex(p);
		local ProfileCard = Def.ActorFrame {
			LoadFont("Common Large") .. {
				Text=profile:GetDisplayName();
				InitCommand=cmd(xy,34/2,-10;zoom,0.4;ztest,true,maxwidth,(200-34-4)/0.4);
			};
			LoadFont("Common Normal") .. {
				InitCommand=cmd(xy,34/2,8;zoom,0.5;vertspacing,-8;ztest,true;maxwidth,(200-34-4)/0.5);
				BeginCommand=function(self)
					local numSongsPlayed = profile:GetNumTotalSongsPlayed();
					local s = numSongsPlayed == 1 and "Song" or "Songs";
					-- todo: localize
					self:settext( numSongsPlayed.." "..s.." Played" );
				end;
			};

			Def.Sprite {
				InitCommand=cmd(visible,true;halign,0;xy,-98,-2;ztest,true;);
				BeginCommand=cmd(queuecommand,"ModifyAvatar");
				ModifyAvatarCommand=function(self)
					self:finishtweening();
					self:LoadBackground(THEME:GetPathG("","../"..getAvatarPathFromProfileID(profileID)));
					self:zoomto(30,30)
				end;	
			};

		};
		t[#t+1]=ProfileCard;
	end;

	return t;
end;

function LoadCard(cColor)
	local t = Def.ActorFrame {
		Def.Quad {
			InitCommand=cmd(zoomto,200+4,230+4);
			OnCommand=cmd(diffuse,color("1,1,1,1"));
		};
		Def.Quad {
			InitCommand=cmd(zoomto,200,230);
			OnCommand=cmd(diffusealpha,0.5;diffuse,cColor);
		};
	};
	return t
end
function LoadPlayerStuff(Player)
	local t = {};

	local pn = (Player == PLAYER_1) and 1;

	t[#t+1] = Def.ActorFrame {
		Name = 'JoinFrame';
		LoadCard(Color('Purple'));
		LoadFont("Common Normal") .. {
			Text="Press &START; to join.";
			InitCommand=cmd(shadowlength,1);
			OnCommand=cmd(diffuseshift;effectcolor1,Color('White');effectcolor2,color("0.5,0.5,0.5"));
		};
	};
	
	t[#t+1] = Def.ActorFrame {
		Name = 'BigFrame';
		LoadCard(PlayerColor(Player));
	};
	t[#t+1] = Def.ActorFrame {
		Name = 'SmallFrame';
		InitCommand=cmd(y,-2);
		Def.Quad {
			InitCommand=cmd(zoomto,200,40+2);
			OnCommand=cmd(diffusealpha,0.3);
		};
	};

	t[#t+1] = Def.ActorScroller{
		Name = 'Scroller';
		NumItemsToDraw=6;
		OnCommand=cmd(y,1;SetFastCatchup,true;SetMask,200,58;SetSecondsPerItem,0.15);
		TransformFunction=function(self, offset, itemIndex, numItems)
			local focus = scale(math.abs(offset),0,2,1,0);
			self:visible(false);
			self:y(math.floor( offset*40 ));
		end;
		children = GetLocalProfiles();
	};
	
	t[#t+1] = Def.ActorFrame {
		Name = "EffectFrame";
	};
	t[#t+1] = LoadFont("Common Normal") .. {
		Name = 'SelectedProfileText';
		InitCommand=cmd(y,160;shadowlength,1;);
	};

	return t;
end;

function UpdateInternal3(self, Player)
	local pn = (Player == PLAYER_1) and 1;
	local frame = self:GetChild(string.format('P%uFrame', pn));
	local scroller = frame:GetChild('Scroller');
	local seltext = frame:GetChild('SelectedProfileText');
	local joinframe = frame:GetChild('JoinFrame');
	local smallframe = frame:GetChild('SmallFrame');
	local bigframe = frame:GetChild('BigFrame');

	if GAMESTATE:IsHumanPlayer(Player) then
		frame:visible(true);
		if MEMCARDMAN:GetCardState(Player) == 'MemoryCardState_none' then
			--using profile if any
			joinframe:visible(false);
			smallframe:visible(true);
			bigframe:visible(true);
			seltext:visible(true);
			scroller:visible(true);
			local ind = SCREENMAN:GetTopScreen():GetProfileIndex(Player);
			if ind > 0 then
				scroller:SetDestinationItem(ind-1);
				seltext:settext(PROFILEMAN:GetLocalProfileFromIndex(ind-1):GetDisplayName());
			else
				if SCREENMAN:GetTopScreen():SetProfileIndex(Player, 1) then
					scroller:SetDestinationItem(0);
					self:queuecommand('UpdateInternal2');
				else
					joinframe:visible(true);
					smallframe:visible(false);
					bigframe:visible(false);
					scroller:visible(false);
					seltext:settext('No profile');
				end;
			end;
		else
			--using card
			smallframe:visible(false);
			scroller:visible(false);
			seltext:settext('CARD');
			SCREENMAN:GetTopScreen():SetProfileIndex(Player, 0);
		end;
	else
		joinframe:visible(true);
		scroller:visible(false);
		seltext:visible(false);
		smallframe:visible(false);
		bigframe:visible(false);
	end;
end;

local t = Def.ActorFrame {}

t[#t+1] = Def.ActorFrame{
	StorageDevicesChangedMessageCommand=function(self, params)
		self:queuecommand('UpdateInternal2');
	end;

	CodeMessageCommand = function(self, params)
		if params.Name == 'Start' or params.Name == 'Center' then
			MESSAGEMAN:Broadcast("StartButton");
			if not GAMESTATE:IsHumanPlayer(params.PlayerNumber) then
				SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, -1);
			else
				SCREENMAN:GetTopScreen():Finish();
			end;
		end;
		if params.Name == 'Up' or params.Name == 'Up2' or params.Name == 'DownLeft' then
			if GAMESTATE:IsHumanPlayer(params.PlayerNumber) then
				local ind = SCREENMAN:GetTopScreen():GetProfileIndex(params.PlayerNumber);
				if ind > 1 then
					if SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, ind - 1 ) then
						MESSAGEMAN:Broadcast("DirectionButton");
						self:queuecommand('UpdateInternal2');
					end;
				end;
			end;
		end;
		if params.Name == 'Down' or params.Name == 'Down2' or params.Name == 'DownRight' then
			if GAMESTATE:IsHumanPlayer(params.PlayerNumber) then
				local ind = SCREENMAN:GetTopScreen():GetProfileIndex(params.PlayerNumber);
				if ind > 0 then
					if SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, ind + 1 ) then
						MESSAGEMAN:Broadcast("DirectionButton");
						self:queuecommand('UpdateInternal2');
					end;
				end;
			end;
		end;
		if params.Name == 'Back' then
			if GAMESTATE:GetNumPlayersEnabled()==0 then
				SCREENMAN:GetTopScreen():Cancel();
			else
				MESSAGEMAN:Broadcast("BackButton");
				SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, -2);
			end;
		end;
	end;

	PlayerJoinedMessageCommand=function(self, params)
		self:queuecommand('UpdateInternal2');
	end;

	PlayerUnjoinedMessageCommand=function(self, params)
		self:queuecommand('UpdateInternal2');
	end;

	OnCommand=function(self, params)
		self:queuecommand('UpdateInternal2');
	end;

	UpdateInternal2Command=function(self)
		UpdateInternal3(self, PLAYER_1);
	end;

	children = {
		Def.ActorFrame {
			Name = 'P1Frame';
			InitCommand=cmd(x,SCREEN_CENTER_X;y,SCREEN_CENTER_Y);
			OnCommand=cmd(zoom,0;bounceend,0.35;zoom,1);
			OffCommand=cmd(bouncebegin,0.35;zoom,0);
			PlayerJoinedMessageCommand=function(self,param)
				if param.Player == PLAYER_1 then
					(cmd(;zoom,1.15;bounceend,0.175;zoom,1.0;))(self);
				end;
			end;
			children = LoadPlayerStuff(PLAYER_1);
		};
		-- sounds
		LoadActor( THEME:GetPathS("Common","start") )..{
			StartButtonMessageCommand=cmd(play);
		};
		LoadActor( THEME:GetPathS("Common","cancel") )..{
			BackButtonMessageCommand=cmd(play);
		};
		LoadActor( THEME:GetPathS("Common","value") )..{
			DirectionButtonMessageCommand=cmd(play);
		};
	};
};
t[#t+1] = LoadActor("_frame");
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,5,32;halign,0;valign,1;zoom,0.55;diffuse,getMainColor('positive');settext,"Select Profile:";);
}

return t;
