local t = Def.ActorFrame {}
-- Controls the topmost layer of ScreenEvaluation

local song = GAMESTATE:GetCurrentSong()
local steps = GAMESTATE:GetCurrentSteps()
local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1)
local score = pss:GetHighScore()

local judges = {
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss"
}

-- functionally create the judgment things to save space
local function makejudges()
    local t = Def.ActorFrame {}
    local function makejudge(i)
        return LoadFont("Common Normal") .. {
            InitCommand = function(self)
                self:y(10 * i):zoom(0.4)
                self:settextf("%s: %d", getJudgeStrings(judges[i]), score:GetTapNoteScore(judges[i]))
            end
        }
    end
    for k,v in ipairs(judges) do
        t[#t+1] = makejudge(k)
    end
    return t
end

-- the "everything" container
t[#t+1] = Def.ActorFrame {
    InitCommand = function(self)
        -- children are relative to the center of the screen (relative to this position)
        self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
    end,
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:y(-SCREEN_HEIGHT/3)
            self:maxwidth(SCREEN_WIDTH)
            self:settext(song:GetDisplayMainTitle())
        end
    },
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:y(-SCREEN_HEIGHT/3 + 30)
            self:maxwidth(SCREEN_WIDTH)
            self:settext(song:GetDisplayArtist())
        end
    },
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:y(-SCREEN_HEIGHT/3 + 55)
            self:maxwidth(SCREEN_WIDTH)
            self:settext(getDifficulty(steps:GetDifficulty()))
        end
    },

    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:zoom(4)
            self:valign(1)
            self:settextf("%.6f%%", score:GetWifeScore()*100)
        end
    },
    makejudges() .. {
        InitCommand = function(self)
            self:y(50)
        end
    },
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:y(30)
            self:settextf("%5.2f --- %5.2f", steps:GetMSD(getCurRateValue(), 1), score:GetSkillsetSSR("Overall"))
        end
    },
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:y(5):valign(0)
            self:settext(score:GetModifiers() .. ", Judge " .. GetTimingDifficulty())
            self:zoom(.4)
        end
    },
    LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:y(15):valign(0):zoom(.4)
            self:settext(getRateString(getCurRateValue()))
        end
    }
}


return t