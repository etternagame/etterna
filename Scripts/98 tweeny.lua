-- custom tweens :sunglasses:

-- scroll through a number range over a given period of time
function BitmapText:scrollnumbers(lower, upper, periodSeconds)
    local persecond = 60 -- how many times per second we want this to run

    local range = math.abs(upper - lower)
    local interval = range / (periodSeconds * persecond)
    interval = interval * (lower < upper and 1 or -1)
    
    local fnum
    local function stop()
        local top = SCREENMAN:GetTopScreen()
        if top ~= nil then
            top:clearInterval(fnum)
        end
    end

    local iterations = range / math.abs(interval)
    ms.ok(iterations .. " " .. periodSeconds / iterations .. " " .. interval)
    local cn = 0

    fnum = SCREENMAN:GetTopScreen():setInterval(
        function()
            self:settext(math.floor(lower))
            lower = lower + interval
            cn = cn + 1
            if (lower > upper and interval > 0) or (lower < upper and interval < 0) then
                self:settext(upper)
                stop()
                ms.ok(cn)
            end
        end,
        periodSeconds / iterations
    )
    self.numberFunc = fnum
    self.numberFinal = upper

    return self
end

-- shadow the real stoptweening to skip the scrollnumbers tween
function BitmapText:stoptweening()

    if self.numberFunc ~= nil and self.numberFinal ~= nil then
        -- kill the update function
        local top = SCREENMAN:GetTopScreen()
        if top ~= nil then
            top:clearInterval(self.numberFunc)
        end
        -- skip all numbers
        self:settext(self.numberFinal)
    end

    -- stop all other tweens
    Actor.stoptweening(self)

    return self
end

-- shadow the real hurrytweening to skip the scrollnumbers tween
-- we cant reliably change the interval time so just skip all numbers
function BitmapText:hurrytweening(factor)

    if self.numberFunc ~= nil and self.numberFinal ~= nil then
        -- kill the update function
        local top = SCREENMAN:GetTopScreen()
        if top ~= nil then
            top:clearInterval(self.numberFunc)
        end
        -- skip all numbers
        self:settext(self.numberFinal)
    end

    -- pass it on
    Actor.hurrytweening(self, factor)

    return self
end

-- shadow the real finishtweening to skip the scrollnumbers tween
-- we cant reliably change the interval time so just skip all the numbers
function BitmapText:finishtweening()

    if self.numberFunc ~= nil and self.numberFinal ~= nil then
        -- kill the update function
        local top = SCREENMAN:GetTopScreen()
        if top ~= nil then
            top:clearInterval(self.numberFunc)
        end
        -- skip all numbers
        self:settext(self.numberFinal)
    end

    -- pass it on
    Actor.finishtweening(self)

    return self
end