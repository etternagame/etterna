-- later on much of this should be conducted at the point of score saving and the results stored in the chart table itself
-- so we can just grab a "score display priority" table from the chart upon loading it

local isfallbackscoreType

function isFallbackScoreType()
	return isfallbackscoreType
end
	

-- Grab only the scores for the current rate
function getCurRateScores()
	return getCurChart().Scores[getCurRateString()]
end

-- Functions involving grabbing highscores under varying circumstances
function getAlternativeBestRateScores(scoreType)
	local all = getCurChart().Scores
	local rates = tableKeys(all)
	local o
	table.sort(rates)

	for i=#rates, 1, -1 do
		o = getRateScoresBestScore(all[rates[i]], scoreType, "Grade_Failed")	 
		if o then return o end
	end
	
	o = getRateScoresBestScore(getCurRateScores(), scoreType, nil)
	if o then return o end
	
	for i=#rates, 1, -1 do
		o = getRateScoresBestScore(all[rates[i]], scoreType, nil)
		if o then return o end
	end
	return nil
end

function getRateScoresBestScore(RateScores, scoreType, filter)	-- filter by grade
	isfallbackscoreType = false
	if not RateScores then return nil end
	local n, o = -math.huge
	for k,v in pairs(RateScores) do
		if v.Metadata.Grade ~= filter then
			v = v.ScoreTable[scoreType]
			if v and v.Percent > n then
				n, o = v.Percent, k
			end
		end
	end

	if not o then 
		for k,v in pairs(RateScores) do
		if v.Metadata.Grade ~= filter then
			v = v.ScoreTable[getfallbackscoreType()]
			if v and v.Percent > n then
				n, o = v.Percent, k
			end
			if not o then 
				return nil
			end
		end
	end
		isfallbackscoreType = true
	end
	return RateScores[o]
end

function getCurRateScoresBestScore(scoreType)	-- not used atm
	isfallbackscoreType = false
	local RateScores = getCurRateScores()
	
	if not RateScores then return nil end
	local n, o = -math.huge
	for k,v in pairs(RateScores) do
		v = v.ScoreTable[scoreType]
		if v and v.Percent > n then
			n, o = v.Percent, k
		end
	end

	if not o then 
		for k,v in pairs(RateScores) do
			v = v.ScoreTable[getfallbackscoreType()]
			if v and v.Percent > n then
				n, o = v.Percent, k
			end
			if not o then 
				isfallbackscoreType = nil
				return nil
			end
		end
		isfallbackscoreType = true
	end
	return RateScores[o]
end

-- for making the music wheel show all grades... one day... the dream...
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

function getBestGradeBySong(song)
	if not song then return nil end		-- shouldn't ever be the case
	
	local highest = 21
	local indexScore
	local grade = "Grade_None"
	local temp = 0
	local scoretable
	local profile = GetPlayerOrMachineProfile(PLAYER_1)
	local steps4k = song:GetStepsByStepsType('StepsType_Dance_Single')
	
	for ix=1,#steps4k do
		scoretable = profile:GetHighScoreList(song,steps4k[ix]):GetHighScores()
		for i=1, #scoretable do
			indexScore = scoretable[i]
			temp = gradeTiers[indexScore:GetGrade()] or 21
			if temp <= highest and isScoreValid(PLAYER_1,steps4k[ix],indexScore) then
				grade = getScoreGrade(indexScore)
				highest = temp
			end
		end
	end
	return grade
end