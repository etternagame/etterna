local c

local ShowComboAt = THEME:GetMetric("Combo", "ShowComboAt");

local t = Def.ActorFrame {
	InitCommand=cmd(vertalign,bottom),
	LoadFont( "Combo", "numbers" ) .. {
		Name="Number",
		OnCommand = THEME:GetMetric("Combo", "NumberOnCommand")
	},
	LoadFont("Common Normal") .. {
		Name="Label",
		OnCommand = THEME:GetMetric("Combo", "LabelOnCommand")
	},
	InitCommand = function(self)
		c = self:GetChildren()
		c.Number:visible(false)
		c.Label:visible(false)
	end,

	ComboCommand=function(self, param)
		local iCombo = param.Combo
		if not iCombo or iCombo < ShowComboAt then
			c.Number:visible(false)
			c.Label:visible(false)
			return
		end
		
		c.Label:settext("COMBO")
		c.Label:visible(false)

		c.Number:visible(true)
		c.Label:visible(true)
		c.Number:settext(iCombo)
		
		-- FullCombo Rewards
		if param.FullComboW1 then
			c.Number:diffuse(color("#00aeef"))
			c.Number:glowshift()
		elseif param.FullComboW2 then
			c.Number:diffuse(color("#fff568"))
			c.Number:glowshift()
		elseif param.FullComboW3 then
			c.Number:diffuse(color("#a4ff00"))
			c.Number:stopeffect()
		elseif param.Combo then
			c.Number:diffuse(Color("White"))
			c.Number:stopeffect()
			c.Label:diffuse(Color("Blue"))
			c.Label:diffusebottomedge(color("0.75,0.75,0.75,1"))
		else
			c.Number:diffuse(color("#ff0000"))
			c.Number:stopeffect()
			c.Label:diffuse(Color("Red"))
			c.Label:diffusebottomedge(color("0.5,0,0,1"))
		end
	end
}

return t
