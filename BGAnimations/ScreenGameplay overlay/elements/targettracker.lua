local target = playerConfig:get_data().TargetGoal
GAMESTATE:GetPlayerState():SetTargetGoal(target / 100)
local targetTrackerMode = playerConfig:get_data().TargetTrackerMode

local t = Def.ActorFrame {
	Name = "TargetTracker",
	InitCommand = function(self)
		self:xy(MovableValues.TargetTrackerX, MovableValues.TargetTrackerY):zoom(MovableValues.TargetTrackerZoom)
	end,
}

if targetTrackerMode == 0 then
	t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PercentDifferential",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:settextf("")
        end,
        SpottedOffsetCommand = function(self)
            if tDiff >= 0 then
                diffuse(self, positive)
            else
                diffuse(self, negative)
            end
            self:settextf("%5.2f (%5.2f%%)", tDiff, target)
        end
    }
else
	t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PBDifferential",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:settextf("")
        end,
        SpottedOffsetCommand = function(self, msg)
            if pbtarget then
                if tDiff >= 0 then
                    self:diffuse(color("#00ff00"))
                else
                    self:diffuse(negative)
                end
                self:settextf("%5.2f (%5.2f%%)", tDiff, pbtarget * 100)
            else
                if tDiff >= 0 then
                    self:diffuse(positive)
                else
                    self:diffuse(negative)
                end
                self:settextf("%5.2f (%5.2f%%)", tDiff, target)
            end
        end
    }
end

return t