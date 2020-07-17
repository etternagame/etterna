local t = Def.ActorFrame {}
-- Controls the song info relevant children of the ScreenSelectMusic decorations actorframe

local wheelX = 15
local arbitraryWheelXThing = 17
local space = 20
local meter = {}
meter[1] = 0
local steps
local song

-- functionally make skillset rating text to save space
local function makeSSes()
    local ss = Def.ActorFrame {}
    local function makeSS(i)
        return LoadFont("Common Normal") .. {
            InitCommand = function(self)
                self:y(10 * i)
                self:zoom(.3)
                self:halign(0)
            end,
            SetStuffCommand = function(self)
                if not steps or not meter[i] then
                    self:settextf("%s:", ms.SkillSetsTranslated[i])
                else
                    self:settextf("%s: %5.2f", ms.SkillSetsTranslated[i], meter[i])
                end
            end
        }
    end
    for i = 1, #ms.SkillSets do
        ss[#ss+1] = makeSS(i)
    end
    return ss
end

local songinfoLine = 100

t[#t+1] = Def.ActorFrame {
    InitCommand = function(self)
        self:x(wheelX + arbitraryWheelXThing + space + capWideScale(get43size(365),365)-50)
        self:y(20)
    end,
    CurrentStepsP1ChangedMessageCommand = function(self)
        steps = GAMESTATE:GetCurrentSteps()
        song = GAMESTATE:GetCurrentSong()
        if steps then
            meter = {}
            for i = 1, #ms.SkillSets do
                local m = steps:GetMSD(getCurRateValue(), i)
                meter[i] = m
            end
        end
        self:playcommand("SetStuff")
    end,

    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:zoom(.5):halign(0)
            self:settextf("Song Info")
        end
    },
    makeSSes() .. {
        InitCommand = function(self)
            self:y(10)
        end
    },

    Def.BPMDisplay {
		File = THEME:GetPathF("BPMDisplay", "bpm"),
		Name = "BPMDisplay",
		InitCommand = function(self)
			self:xy(songinfoLine + 3, 20):halign(0):zoom(0.3)
		end,
        SetStuffCommand = function(self)
            if song then
				self:visible(true)
				self:SetFromSong(song)
			else
				self:visible(false)
			end
		end
    },
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:xy(songinfoLine, 20):zoom(.3)
            self:settext("BPM:"):halign(1)
        end
    },

	LoadFont("Common Normal") .. {
        Name = "RateDisplay",
		InitCommand = function(self)
			self:xy(songinfoLine, 30):zoom(0.3)
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:settext(getCurRateDisplayString())
		end,
        CodeMessageCommand = function(self, params)
			local rate = getCurRateValue()
			ChangeMusicRate(rate, params)
			self:settext(getCurRateDisplayString())
		end
	}
    
}

return t