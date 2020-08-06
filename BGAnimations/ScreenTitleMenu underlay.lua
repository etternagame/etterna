local t = Def.ActorFrame {Name = "UnderlayFile"}

t[#t+1] = Def.Quad {
    Name = "RightBG",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
        self:diffuse(color("#333333"))
    end
}

-- the grid image we are using is 200x200
-- determine how many images we need to load to cover the screen
local imageheight = 200 -- unused
local imagewidth = 200 -- unused
local adjustedsize = 200 / 1080 * SCREEN_HEIGHT

local verticalcount = math.ceil(SCREEN_HEIGHT / adjustedsize)
local horizontalcount = math.ceil(SCREEN_WIDTH / adjustedsize)

for i = 1,verticalcount do
    for j = 1, horizontalcount do
        t[#t+1] = Def.Sprite {
            Name = "BGCheckerboard_"..i.."_"..j,
            Texture = THEME:GetPathG("", "bg-pattern"),
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy((j-1) * adjustedsize, (i-1) * adjustedsize)
                self:zoomto(adjustedsize, adjustedsize)
            end
        }
    end
end


local gradientwidth = 1104 / 1920 * SCREEN_WIDTH
local gradientheight = SCREEN_HEIGHT
local separatorxpos = 814 / 1920 * SCREEN_WIDTH -- basically the top right edge of the gradient
local separatorthickness = 22 / 1920 * SCREEN_WIDTH -- very slightly fudged due to measuring a diagonal line
local separatorlength = math.sqrt(SCREEN_HEIGHT * SCREEN_HEIGHT + (gradientwidth - separatorxpos) * (gradientwidth - separatorxpos)) + 10 -- hypotenuse


t[#t+1] = Def.Sprite {
    Name = "LeftBG",
    Texture = THEME:GetPathG("", "title-gradient"),
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(gradientwidth, gradientheight)
    end
}

t[#t+1] = Def.Quad {
    Name = "SeparatorShadow",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(separatorthickness * 3, separatorlength)
        self:x(separatorxpos - separatorthickness)
        self:diffuse(color("0,0,0,1"))
        self:fadeleft(1)
        self:faderight(1)
        self:rotationz(-15)
    end
}

t[#t+1] = Def.Quad {
    Name = "Separator",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(separatorthickness, separatorlength)
        self:x(separatorxpos)
        self:rotationz(-15)
    end
}

return t