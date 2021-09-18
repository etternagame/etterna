-- the grid image we are using is 200x200
-- determine how many images we need to load to cover the screen
local adjustedsize = 200 / 1080 * SCREEN_HEIGHT

-- adding 1 to these counts for the moving background stuff, give us more room to work with
local verticalcount = math.ceil(SCREEN_HEIGHT / adjustedsize) + 1
local horizontalcount = math.ceil(SCREEN_WIDTH / adjustedsize) + 1
local checkerboardAnimationSeconds = 7

-- generate the bg checkerboard as a frame
local function bgCheckerBoard()
    local d = Def.ActorFrame {Name = "BGCheckerboardFrame"}

    for i = 1,verticalcount do
        for j = 1, horizontalcount do
            d[#d+1] = Def.Sprite {
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

    return d
end

return Def.ActorFrame {
    Name = "BGContainer",
    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
            self:diffuse(COLORS:getTitleColor("UnderlayBackground"))
            self:diffusealpha(1)
        end
    },
    bgCheckerBoard() .. {
        -- These extra commands will move the checkerboard diagonally up left infinitely
        BeginCommand = function(self)
            self:queuecommand("Animate")
        end,
        AnimateCommand = function(self)
            self:xy(0,0)
            self:linear(checkerboardAnimationSeconds)
            self:xy(-adjustedsize,-adjustedsize)
            self:queuecommand("Animate")
        end
    }
}