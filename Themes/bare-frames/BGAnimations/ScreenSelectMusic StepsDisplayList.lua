return Def.StepsDisplayList {
	Name = "StepsDisplayList",
	CursorP1 = Def.ActorFrame {
		InitCommand = function(self)
			self:x(-128 + 16):player(PLAYER_1)
		end,
		PlayerJoinedMessageCommand = function(self, params)
			if params.Player == PLAYER_1 then
				self:visible(true)
				self:bounceend(0.3):zoom(1)
			end
		end,
		PlayerUnjoinedMessageCommand = function(self, params)
			if params.Player == PLAYER_1 then
				self:visible(true)
				self:bounceend(0.3):zoom(0)
			end
		end,
		LoadFont("Common Normal") ..
			{
				Text = "P1",
				InitCommand = function(self)
					self:x(-4):diffuse(PlayerColor(PLAYER_1)):shadowlength(1)
				end,
				OnCommand = function(self)
					self:zoom(0.75)
				end
			}
	},
	CursorP2 = Def.ActorFrame {
		InitCommand = function(self)
			self:x(128 - 16):player(PLAYER_2)
		end,
		PlayerJoinedMessageCommand = function(self, params)
			if params.Player == PLAYER_2 then
				self:visible(true)
				self:bounceend(0.3):zoom(1)
			end
		end,
		PlayerUnjoinedMessageCommand = function(self, params)
			if params.Player == PLAYER_2 then
				self:visible(true)
				self:bounceend(0.3):zoom(0)
			end
		end,
		LoadFont("Common Normal") ..
			{
				Text = "P2",
				InitCommand = function(self)
					self:x(8):diffuse(PlayerColor(PLAYER_2)):shadowlength(1)
				end,
				OnCommand = function(self)
					self:zoom(0.75)
				end
			}
	},
	CursorP1Frame = Def.Actor {
		ChangeCommand = function(self)
			self:stoptweening():decelerate(0.125)
		end
	},
	CursorP2Frame = Def.Actor {
		ChangeCommand = function(self)
			self:stoptweening():decelerate(0.125)
		end
	}
}
