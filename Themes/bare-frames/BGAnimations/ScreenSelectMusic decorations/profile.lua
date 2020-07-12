local t = Def.ActorFrame {}

local profile = GetPlayerOrMachineProfile(PLAYER_1)
local wheelX = 15
local arbitraryWheelXThing = 17
local space = 20

local function makeSSes()
    local ss = Def.ActorFrame {}
    local function makeSS(i)
        return LoadFont("Common Normal") .. {
            InitCommand = function(self)
                self:y(10 * i)
                self:zoom(.3)
                self:halign(0)
                self:settextf("%s: %5.2f", ms.SkillSetsTranslated[i], profile:GetPlayerSkillsetRating(ms.SkillSets[i]))
            end
        }
    end
    for i = 1, #ms.SkillSets do
        ss[#ss+1] = makeSS(i)
    end
    return ss
end

t[#t+1] = Def.ActorFrame {
    InitCommand = function(self)
        self:x(wheelX + arbitraryWheelXThing + space + capWideScale(get43size(365),365)-50)
        self:y(SCREEN_CENTER_Y + SCREEN_HEIGHT/2 - 20 - 10 * #ms.SkillSets)
    end,

    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:zoom(.5):halign(0)
            self:settextf("Player Rating")
        end
    },
    makeSSes() .. {
        InitCommand = function(self)
            self:y(10)
        end
    }
}

return t