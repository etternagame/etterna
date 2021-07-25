
local measures = {}
local beatcounter = 0
local measure = 1
local thingy = 1
local active = false
local t = Def.ActorFrame {
	InitCommand = function(self)
		self:x(200)
		self:y(200)

		local steps = GAMESTATE:GetCurrentSteps()
		local loot = steps:GetNPSPerMeasure(1)
						
		local peak = 0
		for i = 1, #loot do
			if loot[i] > peak then		
				peak = loot[i]
			end
		end
	
		local m_len = 0
		local m_spd = 0
		local m_start = 0
		for i = 1, #loot do
			if m_len == 0 then
				m_spd = loot[i]
				m_start = i
			end
	
			if math.abs(m_spd - loot[i]) < 2 then
				m_len = m_len + 1
				m_spd = (m_spd + loot[i]) / 2
			elseif m_len > 1 and m_spd > peak / 1.6 then
				measures[#measures + 1] = { m_start, m_len, m_spd }
				m_len = 0
			else
				m_len = 0
			end
		end
	end,
	
	LoadFont("Common Normal") .. {
		OnCommand = function(self)
			self:visible(false)
			self:settext("")

			if measure == measures[thingy][1] then	
				self:playcommand("Dootz")
			end
		end,
		BeatCrossedMessageCommand = function(self)
			if thingy <= #measures then
				beatcounter = beatcounter + 1
                if beatcounter == 4 then
                    measure = measure + 1
                    beatcounter = 0

                    if measure == measures[thingy][1] then
                        self:playcommand("Dootz")
                    end

                    if measure > measures[thingy][1] + measures[thingy][2] then
                        self:playcommand("UnDootz")
                        thingy = thingy + 1
                    end

                    if active then
                        self:playcommand("MeasureCrossed")
                    end
                end
			end
		end,
		DootzCommand = function(self)
			self:visible(true)
			active = true
			self:settext(measure - measures[thingy][1] .. " / " .. measures[thingy][2])
		end,
		MeasureCrossedCommand = function(self)
			self:settext(measure - measures[thingy][1] .. " / " .. measures[thingy][2])
		end,
		UnDootzCommand = function(self)
			self:visible(false)
			active = false
		end
	}
}