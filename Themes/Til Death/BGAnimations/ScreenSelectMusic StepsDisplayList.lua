return Def.StepsDisplayList {
	Name="StepsDisplayList";
 
	CursorP1 = Def.ActorFrame {
		InitCommand=cmd(x,-128+16;player,PLAYER_1);
		PlayerJoinedMessageCommand=function(self, params)
			if params.Player == PLAYER_1 then
				self:visible(true);
				(cmd(zoom,0;bounceend,0.3;zoom,1))(self);
			end;
	 	end;
	 	PlayerUnjoinedMessageCommand=function(self, params)
			if params.Player == PLAYER_1 then
				self:visible(true);
				(cmd(bouncebegin,0.3;zoom,0))(self);
			end;
		end;
		LoadFont("Common Normal") .. {
			Text="P1";
			InitCommand=cmd(x,-4;diffuse,PlayerColor(PLAYER_1);shadowlength,1);
			OnCommand=cmd(zoom,0.75);
		};
	};
	CursorP2 = Def.ActorFrame {
		InitCommand=cmd(x,128-16;player,PLAYER_2);
		PlayerJoinedMessageCommand=function(self, params)
			if params.Player == PLAYER_2 then
				self:visible(true);
				(cmd(zoom,0;bounceend,0.3;zoom,1))(self);
			end;
		end;
		PlayerUnjoinedMessageCommand=function(self, params)
			if params.Player == PLAYER_2 then
				self:visible(true);
				(cmd(bouncebegin,0.3;zoom,0))(self);
			end;
		end;
		LoadFont("Common Normal") .. {
			Text="P2";
			InitCommand=cmd(x,8;diffuse,PlayerColor(PLAYER_2);shadowlength,1);
			OnCommand=cmd(zoom,0.75);
		};
	};
	CursorP1Frame = Def.Actor{
		ChangeCommand=cmd(stoptweening;decelerate,0.125);
	};
	CursorP2Frame = Def.Actor{
		ChangeCommand=cmd(stoptweening;decelerate,0.125);
	};
};