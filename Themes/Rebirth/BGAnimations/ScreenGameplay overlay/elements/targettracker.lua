local target = playerConfig:get_data().TargetGoal
GAMESTATE:GetPlayerState():SetTargetGoal(target / 100)
local targetTrackerMode = playerConfig:get_data().TargetTrackerMode

-- describes the difference between 480p and current resolution (theme elements are resized based on theme height)
-- for the purpose of scaling some hardcoded values for element sizing purposes
-- was extremely lazy here
local GAMEPLAY_SIZING_RATIO = (480 / SCREEN_HEIGHT)

local t = Def.ActorFrame {
    Name = "TargetTracker",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.TargetTrackerX, MovableValues.TargetTrackerY)
        self:zoom(MovableValues.TargetTrackerZoom / GAMEPLAY_SIZING_RATIO)
    end,
}

local aheadColor = COLORS:getGameplayColor("TargetGoalAhead")
local behindColor = COLORS:getGameplayColor("TargetGoalBehind")

if targetTrackerMode == 0 then
    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PercentDifferential",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:settext("")
        end,
        SpottedOffsetCommand = function(self, params)
            local tDiff = params.targetDiff
            if tDiff >= 0 then
                self:diffuse(aheadColor)
            else
                self:diffuse(behindColor)
            end
            self:settextf("%5.2f (%5.2f%%)", tDiff, target)
        end
    }
else
    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PBDifferential",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:settext("")
        end,
        SpottedOffsetCommand = function(self, params)
            local tDiff = params.targetDiff
            if params and params.pbTarget then
                if tDiff >= 0 then
                    self:diffuse(aheadColor)
                else
                    self:diffuse(behindColor)
                end
                self:settextf("%5.2f (%5.2f%%)", tDiff, params.pbTarget * 100)
            else
                -- if set to pb goal but there is no pb, default to the set target value
                if tDiff >= 0 then
                    self:diffuse(aheadColor)
                else
                    self:diffuse(behindColor)
                end
                self:settextf("%5.2f (%5.2f%%)", tDiff, target)
            end
        end
    }
end

return t
