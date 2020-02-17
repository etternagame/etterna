local function CreditsText(pn)
	local text =
		LoadFont(Var "LoadingScreen", "credits") ..
		{
			InitCommand = function(self)
				self:name("Credits" .. PlayerNumberToString(pn))
				ActorUtil.LoadAllCommandsAndSetXY(self, Var "LoadingScreen")
			end,
			UpdateTextCommand = function(self)
				local str = ScreenSystemLayerHelpers.GetCreditsMessage(pn)
				self:settext(str)
			end,
			UpdateVisibleCommand = function(self)
				local screen = SCREENMAN:GetTopScreen()
				local bShow = true
				if screen then
					local sClass = screen:GetName()
					bShow = THEME:GetMetric(sClass, "ShowCreditDisplay")
				end

				self:visible(bShow)
			end
		}
	return text
end

--[[ local function PlayerPane( PlayerNumber ) 
	local t = Def.ActorFrame {
		InitCommand=function(self)
			self:name("PlayerPane" .. PlayerNumberToString(PlayerNumber));
	-- 		ActorUtil.LoadAllCommandsAndSetXY(self,Var "LoadingScreen");
		end
	};
	t[#t+1] = Def.ActorFrame {
		Name = "Background";
		Def.Quad {
			InitCommand=function(self)
				self:zoomto(160,28):queuecommand("On")
			end;
			OnCommand=function(self)
				self:diffuse(PlayerColor(PlayerNumber)):fadebottom(1)
			end;
		};
	};
	t[#t+1] = Def.BitmapText{
		Font="Common Normal";
		Name = "PlayerText";
		InitCommand=function(self)
			self:x(-60):maxwidth(80/0.5):zoom(0.5):queuecommand("On")
		end;
		OnCommand=function(self)
			self:playcommand("Set")
		end;
		SetCommand=function(self)
			local profile = PROFILEMAN:GetProfile( PlayerNumber) or PROFILEMAN:GetMachineProfile()
			if profile then
				self:settext( profile:GetDisplayName() );
			else
				self:settext( "NoProf" );
			end
		end;
	};
	return t
end --]]
--
local t = Def.ActorFrame {}
-- Credits
t[#t + 1] =
	Def.ActorFrame {
	--[[  	PlayerPane( PLAYER_1 ) .. {
		InitCommand=function(self)
			self:x(scale(0.125,0,1,SCREEN_LEFT,SCREEN_WIDTH)):y(SCREEN_BOTTOM-16)
		end	
	}; --]]
	CreditsText(PLAYER_1),
	CreditsText(PLAYER_2)
}
-- Text
t[#t + 1] =
	Def.ActorFrame {
	Def.Quad {
		InitCommand = function(self)
			self:zoomtowidth(SCREEN_WIDTH):zoomtoheight(30):horizalign(left):vertalign(top):y(SCREEN_TOP):diffuse(
				color("0,0,0,0")
			)
		end,
		OnCommand = function(self)
			self:finishtweening():diffusealpha(0.85)
		end,
		OffCommand = function(self)
			self:sleep(3):linear(0.5):diffusealpha(0)
		end
	},
	Def.BitmapText {
		Font = "Common Normal",
		Name = "Text",
		InitCommand = function(self)
			self:maxwidth(750):horizalign(left):vertalign(top):y(SCREEN_TOP + 10):x(SCREEN_LEFT + 10):shadowlength(1):diffusealpha(
				0
			)
		end,
		OnCommand = function(self)
			self:finishtweening():diffusealpha(1):zoom(0.5)
		end,
		OffCommand = function(self)
			self:sleep(3):linear(0.5):diffusealpha(0)
		end
	},
	SystemMessageMessageCommand = function(self, params)
		self:GetChild("Text"):settext(params.Message)
		self:playcommand("On")
		if params.NoAnimate then
			self:finishtweening()
		end
		self:playcommand("Off")
	end,
	HideSystemMessageMessageCommand = function(self)
		self:finishtweening()
	end
}

return t
