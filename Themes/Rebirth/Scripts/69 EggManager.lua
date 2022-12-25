-- eggman
EGGMAN = {}
local seif, salf = Actor.rainbow, Actor.rainbow
local solf = Actor.stopeffect
local function CHECK_FOR_THE_DATE_THE_eGG_THE_THING_DATE_AND_TIME_egg(moth, de)
    local m = MonthOfYear()+1
    local d = DayOfMonth()
    return moth == m and de == d
end
local function RRRRRRRRRRRRRRRRRANGE(m1, d1, m2, d3)
    local m = MonthOfYear()+1
    local d = DayOfMonth()
    return m >= m1 and m <= m2 and d >= d1 and d <= (d3)
end

local function INCUBATION_EGG(md,dm) return function() return CHECK_FOR_THE_DATE_THE_eGG_THE_THING_DATE_AND_TIME_egg(md,dm) end end
local function INCUBPATION_ERIOD(md,dm,mmd,ddm) return function() return RRRRRRRRRRRRRRRRRANGE(md,dm,mmd,ddm) end end

local ninety_eight = (((((69.00 * 34.0) / 35.000) / 36.0000) / 26.0) * 13.7) salf = ninety_eight
-- 4/1
-- 12/20 - 1/1
--

EGGMAN.snowyboy = function() if not INCUBPATION_ERIOD(12,20, 12,31)() then return else local t=Def.ActorFrame{} for i=1,150 do t[#t+1]=Def.Quad{BaseZoomX=1+math.random(8),BaseZoomY=1+math.random(8),
OnCommand = function(self) self:queuecommand("startup")end,startupCommand=function(self)math.random()self:y(-20):sleep(i/5):queuecommand("flake") end, flakeCommand = function(self) local xx = math.random(SCREEN_WIDTH) self:xy(xx,-20):smooth(math.max(1500,math.random(4500))/100):xy(xx+math.random(SCREEN_WIDTH/3)-math.random(SCREEN_WIDTH/3),SCREEN_HEIGHT+15):queuecommand("flake")end} end return t end end local sejf = math.random
EGGMAN.foole = function() if not INCUBATION_EGG(4,1)() then return else local t=Def.ActorFrame{OnCommand=function(self)SCREENMAN:GetTopScreen():rotationy(180):x(SCREEN_WIDTH):queuecommand('step2')end,step2Command=function(self)self:sleep(2):queuecommand("step3")end,step3Command=function(self)SCREENMAN:GetTopScreen():rotationy(0):x(0)end}return t end end
EGGMAN.HLALO = INCUBATION_EGG(10,31)

-- egg

local gega, goo, go = "X2736df0ef238", "6bd54a538e32130", "61ecd00940994"
local gegagoogoo = gega..go..goo
EGGMAN.gegagoogoo = function(self, sell) if sell == gegagoogoo or sejf() > salf then return seif(self) else return solf(self) end end


-- 2do: src/Etterna/Singletons/

--