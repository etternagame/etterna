-- the wifepercent display. it displays the wifepercent

-- i dunno less copy paste whatever bro
local formatstr = "%05.2f%%"
local wifepercentTextSize = 0.3 / GAMEPLAY_SIZING_RATIO

return Def.ActorFrame {
	Name = "DisplayPercent",
	InitCommand = function(self)
        self:xy(MovableValues.DisplayPercentX, MovableValues.DisplayPercentY)
		self:zoom(MovableValues.DisplayPercentZoom)
	end,
    SpottedOffsetCommand = function(self, params)
        local bg = self:GetChild("PercentBacking")
        local perc = self:GetChild("DisplayPercent")

        -- kind of putting all the logic in one place here
        -- modify text then update bg for it
        if params ~= nil and params.wifePercent ~= nil then
            if perc then
                perc:settextf(formatstr, params.wifePercent)
            end
        else
            if perc then
                perc:settextf(formatstr, 0)
            end
        end
        if bg and perc then
            bg:zoomto(perc:GetZoomedWidth() + 4, perc:GetZoomedHeight() + 4)
        end
    end,

	Def.Quad {
        Name = "PercentBacking",
		InitCommand = function(self)
            self:halign(1):valign(0)
            self:diffuse(color("0,0,0,0.4"))
		end
	},
	LoadFont("Common Large") .. {
        Name = "DisplayPercent",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(wifepercentTextSize)
            -- maybe we want to set the text color here
        end,
    }
}