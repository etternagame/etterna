local judges = {"marv", "perf", "great", "good", "boo", "miss"}

local gradeTiers = {
	Grade_Tier01 = 0,
	Grade_Tier02 = 1,
	Grade_Tier03 = 2,
	Grade_Tier04 = 3,
	Grade_Tier05 = 4,
	Grade_Tier06 = 5,
	Grade_Tier07 = 6,
	Grade_Tier08 = 7,
	Grade_Tier09 = 8,
	Grade_Tier10 = 9,
	Grade_Tier11 = 10,
	Grade_Tier12 = 11,
	Grade_Tier13 = 12,
	Grade_Tier14 = 13,
	Grade_Tier15 = 14,
	Grade_Tier16 = 15,
	Grade_Tier17 = 16,
	Grade_Tier18 = 17,
	Grade_Tier19 = 18,
	Grade_Tier20 = 19,
	Grade_Failed = 20
}

local scoreWeight = {
	-- Score Weights for DP score (MAX2)
	TapNoteScore_W1 = 2,
	--PREFSMAN:GetPreference("GradeWeightW1"),					--  2
	TapNoteScore_W2 = 2,
	--PREFSMAN:GetPreference("GradeWeightW2"),					--  2
	TapNoteScore_W3 = 1,
	--PREFSMAN:GetPreference("GradeWeightW3"),					--  1
	TapNoteScore_W4 = 0,
	--PREFSMAN:GetPreference("GradeWeightW4"),					--  0
	TapNoteScore_W5 = -4,
	--PREFSMAN:GetPreference("GradeWeightW5"),					-- -4
	TapNoteScore_Miss = -8,
	--PREFSMAN:GetPreference("GradeWeightMiss"),				-- -8
	HoldNoteScore_Held = 6,
	--PREFSMAN:GetPreference("GradeWeightHeld"),				--  6
	TapNoteScore_HitMine = -8,
	--PREFSMAN:GetPreference("GradeWeightHitMine"),				-- -8
	HoldNoteScore_LetGo = 0,
	--PREFSMAN:GetPreference("GradeWeightLetGo"),				--  0
	HoldNoteScore_MissedHold = 0,
	TapNoteScore_AvoidMine = 0,
	TapNoteScore_CheckpointHit = 0,
	--PREFSMAN:GetPreference("GradeWeightCheckpointHit"),		--  0
	TapNoteScore_CheckpointMiss = 0
	--PREFSMAN:GetPreference("GradeWeightCheckpointMiss"),		--  0
}

local psWeight = {
	-- Score Weights for percentage scores (EX oni)
	TapNoteScore_W1 = 3,
	--PREFSMAN:GetPreference("PercentScoreWeightW1"),
	TapNoteScore_W2 = 2,
	--PREFSMAN:GetPreference("PercentScoreWeightW2"),
	TapNoteScore_W3 = 1,
	--PREFSMAN:GetPreference("PercentScoreWeightW3"),
	TapNoteScore_W4 = 0,
	--PREFSMAN:GetPreference("PercentScoreWeightW4"),
	TapNoteScore_W5 = 0,
	--PREFSMAN:GetPreference("PercentScoreWeightW5"),
	TapNoteScore_Miss = 0,
	--PREFSMAN:GetPreference("PercentScoreWeightMiss"),
	HoldNoteScore_Held = 3,
	--PREFSMAN:GetPreference("PercentScoreWeightHeld"),
	TapNoteScore_HitMine = -2,
	--(0 or -2?) PREFSMAN:GetPreference("PercentScoreWeightHitMine"),
	HoldNoteScore_LetGo = 0,
	--PREFSMAN:GetPreference("PercentScoreWeightLetGo"),
	HoldNoteScore_MissedHold = 0,
	TapNoteScore_AvoidMine = 0,
	TapNoteScore_CheckpointHit = 0,
	--PREFSMAN:GetPreference("PercentScoreWeightCheckpointHit"),
	TapNoteScore_CheckpointMiss = 0
	--PREFSMAN:GetPreference("PercentScoreWeightCheckpointMiss"),
}

local migsWeight = {
	-- Score Weights for MIGS score
	TapNoteScore_W1 = 3,
	TapNoteScore_W2 = 2,
	TapNoteScore_W3 = 1,
	TapNoteScore_W4 = 0,
	TapNoteScore_W5 = -4,
	TapNoteScore_Miss = -8,
	HoldNoteScore_Held = 6,
	TapNoteScore_HitMine = -8,
	HoldNoteScore_LetGo = 0,
	HoldNoteScore_MissedHold = 0,
	TapNoteScore_AvoidMine = 0,
	TapNoteScore_CheckpointHit = 0,
	TapNoteScore_CheckpointMiss = 0
}

-- For offsetplot screen thingy doodle -mina
local ScoreForPlot = nil
function setScoreForPlot(hs)
	ScoreForPlot = hs
end
function getScoreForPlot()
	return ScoreForPlot
end
--Same as above for online scores
local NetScoreForPlot = nil
function setOnlineScoreForPlot(hs)
	NetScoreForPlot = hs
end
function getOnlineScoreForPlot()
	return NetScoreForPlot
end

function getScoreFromTable(hsTable, index)
	return hsTable[index]
end

function getMaxNotes(pn)
	local song = GAMESTATE:GetCurrentSong()
	local steps
	if GAMESTATE:IsPlayerEnabled(pn) then
		steps = GAMESTATE:GetCurrentSteps(pn)
		if steps ~= nil then
			if GAMESTATE:GetCurrentGame():CountNotesSeparately() then
				return steps:GetRadarValues(pn):GetValue("RadarCategory_Notes") or 0
			else
				return steps:GetRadarValues(pn):GetValue("RadarCategory_TapsAndHolds") or 0
			end
		end
	end
	return 0
end

function getMaxHolds(pn)
	local song = GAMESTATE:GetCurrentSong()
	local steps
	if GAMESTATE:IsPlayerEnabled(pn) then
		steps = GAMESTATE:GetCurrentSteps(pn)
		if steps ~= nil then
			return (steps:GetRadarValues(pn):GetValue("RadarCategory_Holds") +
				steps:GetRadarValues(pn):GetValue("RadarCategory_Rolls")) or 0
		end
	end
	return 0
end

function GetPercentDP(score)
	return math.floor(10000 * getScore(score, 1) / getMaxScore(PLAYER_1, 1)) / 100
end

--Gets the highest score possible for the scoretype
function getMaxScore(pn, scoreType) -- dp, ps, migs = 1,2,3 respectively, 0 reverts to default
	local maxNotes = getMaxNotes(pn)
	local maxHolds = getMaxHolds(pn)
	if scoreType == 0 or scoreType == nil then
		scoreType = themeConfig:get_data().global.DefaultScoreType
	end

	if scoreType == 1 or scoreType >= 4 then
		-- maximum DP
		return (maxNotes * scoreWeight["TapNoteScore_W1"] + maxHolds * scoreWeight["HoldNoteScore_Held"])
	elseif scoreType == 2 then
		return (maxNotes * psWeight["TapNoteScore_W1"] + maxHolds * psWeight["HoldNoteScore_Held"]) -- maximum %score DP
	elseif scoreType == 3 then
		return (maxNotes * migsWeight["TapNoteScore_W1"] + maxHolds * migsWeight["HoldNoteScore_Held"])
	else
		return "????"
	end
end

function getGradeThreshold(pn, grade)
	local maxScore = getMaxScore(pn, 1)
	if grade == "Grade_Failed" then
		return 0
	else
		return math.ceil(maxScore * THEME:GetMetric("PlayerStageStats", grade:gsub("Grade_", "GradePercent")))
	end
end

function getGradeFamilyForMidGrade(grade)
	if grade == "Grade_Tier02" or grade == "Grade_Tier03" or grade == "Grade_Tier04" then
		-- quads
		return "Grade_Tier04"
	elseif grade == "Grade_Tier05" or grade == "Grade_Tier06" or grade == "Grade_Tier07" then
		return "Grade_Tier07"
		-- triples
	elseif grade == "Grade_Tier08" or grade == "Grade_Tier09" or grade == "Grade_Tier10" then
		-- doubles
		return "Grade_Tier10"
	elseif grade == "Grade_Tier11" or grade == "Grade_Tier12" or grade == "Grade_Tier13" then
		-- singles
		return "Grade_Tier13"
	else
		-- quint and others
		return grade
	end
end

function getNearbyGrade(pn, DPScore, grade)
	local nextGrade
	local gradeScore = 0
	local nextGradeScore = 0

	if grade == "Grade_Tier01" or grade == "Grade_Tier02" or grade == "Grade_Tier03" or grade == "Grade_Tier04" then
		return grade, 0
	elseif grade == "Grade_Failed" then
		return "Grade_Tier16", DPScore
	elseif grade == "Grade_None" then
		return "Grade_Tier16", 0
	else
		nextGrade = string.format("Grade_Tier%02d", (tonumber(grade:sub(-2)) - 1))
		gradeScore = getGradeThreshold(pn, grade)
		nextGradeScore = getGradeThreshold(pn, nextGrade)

		curGradeDiff = DPScore - gradeScore
		nextGradeDiff = DPScore - nextGradeScore

		if math.abs(curGradeDiff) < math.abs(nextGradeDiff) then
			return grade, curGradeDiff
		else
			return nextGrade, nextGradeDiff
		end
	end
end

function getScoreGrade(score)
	if score ~= nil then
		return score:GetGrade()
	else
		return "Grade_None"
	end
end

function getScoreMaxCombo(score)
	if score ~= nil then
		return score:GetMaxCombo()
	else
		return 0
	end
end

function getScoreDate(score)
	if score ~= nil then
		return score:GetDate()
	else
		return ""
	end
end

function getScoreTapNoteScore(score, tns)
	if score ~= nil then
		return score:GetTapNoteScore(tns)
	else
		return 0
	end
end

function getScoreHoldNoteScore(score, tns)
	if score ~= nil then
		return score:GetHoldNoteScore(tns)
	else
		return 0
	end
end

function getScoreMissCount(score)
	return getScoreTapNoteScore(score, "TapNoteScore_Miss") + getScoreTapNoteScore(score, "TapNoteScore_W5") +
		getScoreTapNoteScore(score, "TapNoteScore_W4")
end

function getScore(score, scoreType)
	if scoreType == 0 or scoreType == nil then
		scoreType = themeConfig:get_data().global.DefaultScoreType
	end

	if scoreType == 1 or scoreType >= 4 then
		return getScoreTapNoteScore(score, "TapNoteScore_W1") * scoreWeight["TapNoteScore_W1"] +
			getScoreTapNoteScore(score, "TapNoteScore_W2") * scoreWeight["TapNoteScore_W2"] +
			getScoreTapNoteScore(score, "TapNoteScore_W3") * scoreWeight["TapNoteScore_W3"] +
			getScoreTapNoteScore(score, "TapNoteScore_W4") * scoreWeight["TapNoteScore_W4"] +
			getScoreTapNoteScore(score, "TapNoteScore_W5") * scoreWeight["TapNoteScore_W5"] +
			getScoreTapNoteScore(score, "TapNoteScore_Miss") * scoreWeight["TapNoteScore_Miss"] +
			getScoreTapNoteScore(score, "TapNoteScore_CheckpointHit") * scoreWeight["TapNoteScore_CheckpointHit"] +
			getScoreTapNoteScore(score, "TapNoteScore_CheckpointMiss") * scoreWeight["TapNoteScore_CheckpointMiss"] +
			getScoreTapNoteScore(score, "TapNoteScore_HitMine") * scoreWeight["TapNoteScore_HitMine"] +
			getScoreTapNoteScore(score, "TapNoteScore_AvoidMine") * scoreWeight["TapNoteScore_AvoidMine"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_LetGo") * scoreWeight["HoldNoteScore_LetGo"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_Held") * scoreWeight["HoldNoteScore_Held"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_MissedHold") * scoreWeight["HoldNoteScore_MissedHold"]
	elseif scoreType == 2 then
		return getScoreTapNoteScore(score, "TapNoteScore_W1") * psWeight["TapNoteScore_W1"] +
			getScoreTapNoteScore(score, "TapNoteScore_W2") * psWeight["TapNoteScore_W2"] +
			getScoreTapNoteScore(score, "TapNoteScore_W3") * psWeight["TapNoteScore_W3"] +
			getScoreTapNoteScore(score, "TapNoteScore_W4") * psWeight["TapNoteScore_W4"] +
			getScoreTapNoteScore(score, "TapNoteScore_W5") * psWeight["TapNoteScore_W5"] +
			getScoreTapNoteScore(score, "TapNoteScore_Miss") * psWeight["TapNoteScore_Miss"] +
			getScoreTapNoteScore(score, "TapNoteScore_CheckpointHit") * psWeight["TapNoteScore_CheckpointHit"] +
			getScoreTapNoteScore(score, "TapNoteScore_CheckpointMiss") * psWeight["TapNoteScore_CheckpointMiss"] +
			getScoreTapNoteScore(score, "TapNoteScore_HitMine") * psWeight["TapNoteScore_HitMine"] +
			getScoreTapNoteScore(score, "TapNoteScore_AvoidMine") * psWeight["TapNoteScore_AvoidMine"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_LetGo") * psWeight["HoldNoteScore_LetGo"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_Held") * psWeight["HoldNoteScore_Held"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_MissedHold") * psWeight["HoldNoteScore_MissedHold"]
	elseif scoreType == 3 then
		return getScoreTapNoteScore(score, "TapNoteScore_W1") * migsWeight["TapNoteScore_W1"] +
			getScoreTapNoteScore(score, "TapNoteScore_W2") * migsWeight["TapNoteScore_W2"] +
			getScoreTapNoteScore(score, "TapNoteScore_W3") * migsWeight["TapNoteScore_W3"] +
			getScoreTapNoteScore(score, "TapNoteScore_W4") * migsWeight["TapNoteScore_W4"] +
			getScoreTapNoteScore(score, "TapNoteScore_W5") * migsWeight["TapNoteScore_W5"] +
			getScoreTapNoteScore(score, "TapNoteScore_Miss") * migsWeight["TapNoteScore_Miss"] +
			getScoreTapNoteScore(score, "TapNoteScore_CheckpointHit") * migsWeight["TapNoteScore_CheckpointHit"] +
			getScoreTapNoteScore(score, "TapNoteScore_CheckpointMiss") * migsWeight["TapNoteScore_CheckpointMiss"] +
			getScoreTapNoteScore(score, "TapNoteScore_HitMine") * migsWeight["TapNoteScore_HitMine"] +
			getScoreTapNoteScore(score, "TapNoteScore_AvoidMine") * migsWeight["TapNoteScore_AvoidMine"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_LetGo") * migsWeight["HoldNoteScore_LetGo"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_Held") * migsWeight["HoldNoteScore_Held"] +
			getScoreHoldNoteScore(score, "HoldNoteScore_MissedHold") * migsWeight["HoldNoteScore_MissedHold"]
	else
		return 0
	end
end

------------------------------------------------
local function GetWifeScoreOrConvertFromDP(score)
	local o = score:GetWifeScore()
	if o > 0 then
		return o
	end
	return score:ConvertDpToWife()
end

-- Rate filter stuff --
local function scoreComparator(scoreA, scoreB)
	local a = GetWifeScoreOrConvertFromDP(scoreA)
	local b = GetWifeScoreOrConvertFromDP(scoreB)
	if a == b then
		return scoreA:GetWifeScore() > scoreB:GetWifeScore()
	end
	return a > b
end

function SortScores(hsTable)
	table.sort(hsTable, scoreComparator)
	return hsTable
end

-- returns a string corresponding to the rate mod used in the highscore.
function getRate(score)
	-- gets the rate mod used in highscore. doesn't work if ratemod has a different name
	local mods = score:GetModifiers()
	if string.find(mods, "Haste") ~= nil then
		return "Haste"
	elseif string.find(mods, "xMusic") == nil then
		return "1.0x"
	else
		return (string.match(mods, "%d+%.%d+xMusic")):sub(1, -6)
	end
end

-- returns the index of the highscore in a given highscore table.
function getHighScoreIndex(hsTable, score)
	for k, v in ipairs(hsTable) do
		if v:GetDate() == score:GetDate() then
			return k
		end
	end
	return 0
end

function getScoresByKey(pn)
	local song = GAMESTATE:GetCurrentSong()
	local profile
	local steps
	if GAMESTATE:IsPlayerEnabled(pn) then
		profile = GetPlayerOrMachineProfile(pn)
		steps = GAMESTATE:GetCurrentSteps(pn)
		if profile ~= nil and steps ~= nil and song ~= nil then
			return SCOREMAN:GetScoresByKey(steps:GetChartKey())
		end
	end
	return nil
end

-- Returns a table containing tables containing scores for each ratemod used.
function getRateTable()
	local o = getScoresByKey(PLAYER_1)
	if not o then
		return nil
	end

	for k, v in pairs(o) do
		o[k] = o[k]:GetScores()
	end

	return o
end

function getUsedRates(rtTable)
	local rates = {}
	local initIndex = 1
	if rtTable ~= nil then
		for k, v in pairs(rtTable) do
			rates[#rates + 1] = k
		end
		table.sort(
			rates,
			function(a, b)
				a = a:gsub("x", "")
				b = b:gsub("x", "")
				return a < b
			end
		)
		for i = 1, #rates do
			if rates[i] == "1.0x" or rates[i] == "All" then
				initIndex = i
			end
		end
	end
	return rates, initIndex
end

function wife2(maxms, ts)
	local avedeviation = 95 * ts
	local y = 1 - (2 ^ (-1 * maxms * maxms / (avedeviation * avedeviation)))
	y = y ^ 2
	return (2 - -8) * (1 - y) + -8
end

-- For Window-based Scoring
function getRescoredJudge(offsetVector, judgeScale, judge)
	local tso = ms.JudgeScalers
	local ts = tso[judgeScale]
	local windows = {22.5, 45.0, 90.0, 135.0, 180.0, 500.0}
	local lowerBound = judge > 1 and windows[judge - 1] * ts or -1.0
	local upperBound = judge == 5 and math.max(windows[judge] * ts, 180.0) or windows[judge] * ts
	local judgeCount = 0

	if judge > 5 then
		lowerBound = math.max(lowerBound, 180.0)
		for i = 1, #offsetVector do
			x = math.abs(offsetVector[i])
			if (x > lowerBound) then
				judgeCount = judgeCount + 1
			end
		end
	else
		for i = 1, #offsetVector do
			x = math.abs(offsetVector[i])
			if (x > lowerBound and x <= upperBound) then
				judgeCount = judgeCount + 1
			end
		end
	end
	return judgeCount
end

-- For Window-based Scoring
function getRescoredCustomJudge(offsetVector, windows, judge)
	local lowerBound = judge > 1 and windows[judges[judge - 1]] or -1.0
	local upperBound = windows[judges[judge]]
	local judgeCount = 0

	if judge > 5 then
		for i = 1, #offsetVector do
			x = math.abs(offsetVector[i])
			if (x > lowerBound) then
				judgeCount = judgeCount + 1
			end
		end
	else
		for i = 1, #offsetVector do
			x = math.abs(offsetVector[i])
			if (x > lowerBound and x <= upperBound) then
				judgeCount = judgeCount + 1
			end
		end
	end
	return judgeCount
end

-- For Millisecond-based Scoring
function getRescoredWifeJudge(judgeScale, rst)
	local tso = ms.JudgeScalers
	local ts = tso[judgeScale]
	local p = 0.0
	for i = 1, #rst["dvt"] do
		p = p + wife2(rst["dvt"][i], ts)
	end
	p = p + (rst["holdsMissed"] * -6)
	p = p + (rst["minesHit"] * -8)
	return (p / (rst["totalTaps"] * 2)) * 100.0
end

-- For Window-based Scoring
function getRescoredCustomPercentage(customWindows, rst)
	local p = 0.0
	local weights = customWindows.judgeWeights
	local windows = customWindows.judgeWindows
	local holdsMissed = totalHolds - holdsHit
	for i = 1, 6 do
		p = p + (getRescoredCustomJudge(offsetVector, windows, i) * weights[judges[i]])
	end
	p = p + (holdsHit * rst["holdsHit"])
	p = p + (holdsMissed * rst["holdsMissed"])
	p = p + (minesHit * rst["minesHit"])
	p = p / ((totalNotes * weights.marv) + (rst["totalHolds"] * weights.holdHit))
	return p * 100.0
end

function GetDisplayScoreByFilter(perc, CurRate) -- moved from wifetwirl, displays the score for the current rate if there is one, 
	local rtTable = getRateTable()				-- if not it looks for what might plausibly be your best by going down each rate
	if not rtTable then
		return nil
	end

	local rates = tableKeys(rtTable)
	local scores, score

	if CurRate then
		local tmp = getCurRateString()
		if tmp == "1x" then
			tmp = "1.0x"
		end
		if tmp == "2x" then
			tmp = "2.0x"
		end
		rates = {tmp}
		if not rtTable[rates[1]] then
			return nil
		end
	end

	table.sort(
		rates,
		function(a, b)
			a = a:gsub("x", "")
			b = b:gsub("x", "")
			return a < b
		end
	)
	for i = #rates, 1, -1 do
		scores = rtTable[rates[i]]
		local bestscore = 0
		local index

		for ii = 1, #scores do
			score = scores[ii]
			if score:ConvertDpToWife() > bestscore then
				index = ii
				bestscore = score:ConvertDpToWife()
			end
		end

		if index and scores[index]:GetWifeScore() == 0 and GetPercentDP(scores[index]) > perc * 100 then
			return scores[index]
		end

		if bestscore > perc then
			return scores[index]
		end
	end
end

function GetDisplayScore()	-- wrapper for above that prioritizes current rate's pb > any rate 90% > any rate 50% > any score any rate
	local score
	score = GetDisplayScoreByFilter(0, true)

	if not score then
		score = GetDisplayScoreByFilter(0.9, false)
	end
	if not score then
		score = GetDisplayScoreByFilter(0.5, false)
	end
	if not score then
		score = GetDisplayScoreByFilter(0, false)
	end
	return score
end

-- erf constants
a1 =  0.254829592
a2 = -0.284496736
a3 =  1.421413741
a4 = -1.453152027
a5 =  1.061405429
p  =  0.3275911


function erf(x)
    -- Save the sign of x
    sign = 1
    if x < 0 then
        sign = -1
    end
    x = math.abs(x)

    -- A&S formula 7.1.26
    t = 1.0/(1.0 + p*x)
    y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*math.exp(-x*x)

    return sign*y
end
function wife3(maxms, ts, version)

	local max_points = 2
	local miss_weight = -5.5
	local ridic = 5 * ts
	local max_boo_weight = 180 * ts
	local ts_pow = 0.75
	local zero = 65 * (ts^ts_pow)
	local power = 2.5
	local dev = 22.7 * (ts^ts_pow)

	-- case handling
	if maxms <= ridic then			-- anything below this (judge scaled) threshold is counted as full pts
		return max_points
	elseif maxms <= zero then			-- ma/pa region, exponential
			return max_points * erf((zero - maxms) / dev)
	elseif maxms <= max_boo_weight then -- cb region, linear
		return (maxms - zero) * miss_weight / (max_boo_weight - zero)
	else							-- we can just set miss values manually
		return miss_weight			-- technically the max boo is always 180 above j4 however this is immaterial to the
	end								-- purpose of the scoring curve, which is to assign point values
end

-- holy shit this is fugly
function getRescoredWife3Judge(version, judgeScale, rst)
	local tso = ms.JudgeScalers
	local ts = tso[judgeScale]
	local p = 0.0
	for i = 1, #rst["dvt"] do							-- wife2 does not require abs due to ^2 but this does
		p = p + wife3(math.abs(rst["dvt"][i]), ts, version)	
	end
	p = p + (rst["holdsMissed"] * -4.5)
	p = p + (rst["minesHit"] * -7)
	return (p / (rst["totalTaps"] * 2)) * 100.0
end