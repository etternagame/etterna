
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

local scoreWeight =  { -- Score Weights for DP score (MAX2)
	TapNoteScore_W1				= 2,--PREFSMAN:GetPreference("GradeWeightW1"),					--  2
	TapNoteScore_W2				= 2,--PREFSMAN:GetPreference("GradeWeightW2"),					--  2
	TapNoteScore_W3				= 1,--PREFSMAN:GetPreference("GradeWeightW3"),					--  1
	TapNoteScore_W4				= 0,--PREFSMAN:GetPreference("GradeWeightW4"),					--  0
	TapNoteScore_W5				= -4,--PREFSMAN:GetPreference("GradeWeightW5"),					-- -4
	TapNoteScore_Miss			= -8,--PREFSMAN:GetPreference("GradeWeightMiss"),				-- -8
	HoldNoteScore_Held			= 6,--PREFSMAN:GetPreference("GradeWeightHeld"),				--  6
	TapNoteScore_HitMine		= -8,--PREFSMAN:GetPreference("GradeWeightHitMine"),				-- -8
	HoldNoteScore_LetGo			= 0,--PREFSMAN:GetPreference("GradeWeightLetGo"),				--  0
	HoldNoteScore_MissedHold	 = 0,
	TapNoteScore_AvoidMine		= 0,
	TapNoteScore_CheckpointHit	= 0,--PREFSMAN:GetPreference("GradeWeightCheckpointHit"),		--  0
	TapNoteScore_CheckpointMiss = 0--PREFSMAN:GetPreference("GradeWeightCheckpointMiss"),		--  0
}

local psWeight =  { -- Score Weights for percentage scores (EX oni)
	TapNoteScore_W1			= 3,--PREFSMAN:GetPreference("PercentScoreWeightW1"),
	TapNoteScore_W2			= 2,--PREFSMAN:GetPreference("PercentScoreWeightW2"),
	TapNoteScore_W3			= 1,--PREFSMAN:GetPreference("PercentScoreWeightW3"),
	TapNoteScore_W4			= 0,--PREFSMAN:GetPreference("PercentScoreWeightW4"),
	TapNoteScore_W5			= 0,--PREFSMAN:GetPreference("PercentScoreWeightW5"),
	TapNoteScore_Miss			= 0,--PREFSMAN:GetPreference("PercentScoreWeightMiss"),
	HoldNoteScore_Held			= 3,--PREFSMAN:GetPreference("PercentScoreWeightHeld"),
	TapNoteScore_HitMine			= -2,--(0 or -2?) PREFSMAN:GetPreference("PercentScoreWeightHitMine"),
	HoldNoteScore_LetGo			= 0,--PREFSMAN:GetPreference("PercentScoreWeightLetGo"),
	HoldNoteScore_MissedHold	 = 0,
	TapNoteScore_AvoidMine		= 0,
	TapNoteScore_CheckpointHit		= 0,--PREFSMAN:GetPreference("PercentScoreWeightCheckpointHit"),
	TapNoteScore_CheckpointMiss 	= 0--PREFSMAN:GetPreference("PercentScoreWeightCheckpointMiss"),
}

local migsWeight =  { -- Score Weights for MIGS score
	TapNoteScore_W1			= 3,
	TapNoteScore_W2			= 2,
	TapNoteScore_W3			= 1,
	TapNoteScore_W4			= 0,
	TapNoteScore_W5			= -4,
	TapNoteScore_Miss			= -8,
	HoldNoteScore_Held			= 6,
	TapNoteScore_HitMine			= -8,
	HoldNoteScore_LetGo			= 0,
	HoldNoteScore_MissedHold	 = 0,
	TapNoteScore_AvoidMine		= 0,
	TapNoteScore_CheckpointHit		= 0,
	TapNoteScore_CheckpointMiss 	= 0
}

-- For offsetplot screen thingy doodle -mina
local ScoreForPlot = nil
function setScoreForPlot(hs) ScoreForPlot = hs end
function getScoreForPlot() return ScoreForPlot end



function getScoreFromTable(hsTable,index)
	return hsTable[index]
end

function getScoreFromPn(pn,index)
	local hsTable = getScoreList(pn)
	return getScore(hsTable,index)
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
			return  (steps:GetRadarValues(pn):GetValue("RadarCategory_Holds") + steps:GetRadarValues(pn):GetValue("RadarCategory_Rolls")) or 0
		end
	end
	return 0
end

function GetPercentDP(score)
	return math.floor(10000*getScore(score,1)/getMaxScore(PLAYER_1, 1))/100
end

--Gets the highest score possible for the scoretype
function getMaxScore(pn,scoreType) -- dp, ps, migs = 1,2,3 respectively, 0 reverts to default
	local maxNotes = getMaxNotes(pn)
	local maxHolds = getMaxHolds(pn)
	if scoreType == 0 or scoreType == nil then
		scoreType = themeConfig:get_data().global.DefaultScoreType
	end;

	if scoreType == 1 or scoreType >= 4 then
		return (maxNotes*scoreWeight["TapNoteScore_W1"]+maxHolds*scoreWeight["HoldNoteScore_Held"])-- maximum DP
	elseif scoreType == 2 then
		return (maxNotes*psWeight["TapNoteScore_W1"]+maxHolds*psWeight["HoldNoteScore_Held"]) -- maximum %score DP
	elseif scoreType == 3 then
		return (maxNotes*migsWeight["TapNoteScore_W1"]+maxHolds*migsWeight["HoldNoteScore_Held"])
	else
		return "????"
	end
end

function getGradeThreshold(pn,grade)
	local maxScore = getMaxScore(pn,1)
	if grade == "Grade_Failed" then
		return 0
	else
		return math.ceil(maxScore*THEME:GetMetric("PlayerStageStats",grade:gsub("Grade_","GradePercent")))
	end
end

function getNearbyGrade(pn,DPScore,grade)
	local nextGrade
	local gradeScore = 0
	local nextGradeScore = 0
	if grade == "Grade_Tier01" or grade == "Grade_Tier02" then
		return grade,0
	elseif grade == "Grade_Failed" then
		return "Grade_Tier07",DPScore
	elseif grade == "Grade_None" then
		return "Grade_Tier07",0
	else
		nextGrade = string.format("Grade_Tier%02d",(tonumber(grade:sub(-2))-1))
		gradeScore = getGradeThreshold(pn,grade)
		nextGradeScore = getGradeThreshold(pn,nextGrade)

		curGradeDiff = DPScore - gradeScore
		nextGradeDiff = DPScore - nextGradeScore

		if math.abs(curGradeDiff) < math.abs(nextGradeDiff) then
			return grade,curGradeDiff
		else
			return nextGrade,nextGradeDiff
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

function getScoreTapNoteScore(score,tns)
	if score ~= nil then
		return score:GetTapNoteScore(tns)
	else
		return 0
	end
end

function getScoreHoldNoteScore(score,tns)
	if score ~= nil then
		return score:GetHoldNoteScore(tns)
	else
		return 0
	end
end

function getScoreMissCount(score)
	return getScoreTapNoteScore(score,"TapNoteScore_Miss") + getScoreTapNoteScore(score,"TapNoteScore_W5") + getScoreTapNoteScore(score,"TapNoteScore_W4")
end

function getScore(score,scoreType)
	if scoreType == 0 or scoreType == nil then
		scoreType = themeConfig:get_data().global.DefaultScoreType
	end

	if scoreType == 1 or scoreType >= 4 then
		return 
		getScoreTapNoteScore(score,"TapNoteScore_W1")*scoreWeight["TapNoteScore_W1"]+
		getScoreTapNoteScore(score,"TapNoteScore_W2")*scoreWeight["TapNoteScore_W2"]+
		getScoreTapNoteScore(score,"TapNoteScore_W3")*scoreWeight["TapNoteScore_W3"]+
		getScoreTapNoteScore(score,"TapNoteScore_W4")*scoreWeight["TapNoteScore_W4"]+
		getScoreTapNoteScore(score,"TapNoteScore_W5")*scoreWeight["TapNoteScore_W5"]+
		getScoreTapNoteScore(score,"TapNoteScore_Miss")*scoreWeight["TapNoteScore_Miss"]+
		getScoreTapNoteScore(score,"TapNoteScore_CheckpointHit")*scoreWeight["TapNoteScore_CheckpointHit"]+
		getScoreTapNoteScore(score,"TapNoteScore_CheckpointMiss")*scoreWeight["TapNoteScore_CheckpointMiss"]+
		getScoreTapNoteScore(score,"TapNoteScore_HitMine")*scoreWeight["TapNoteScore_HitMine"]+
		getScoreTapNoteScore(score,"TapNoteScore_AvoidMine")*scoreWeight["TapNoteScore_AvoidMine"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_LetGo")*scoreWeight["HoldNoteScore_LetGo"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_Held")*scoreWeight["HoldNoteScore_Held"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_MissedHold")*scoreWeight["HoldNoteScore_MissedHold"]

	elseif scoreType == 2 then
		return 
		getScoreTapNoteScore(score,"TapNoteScore_W1")*psWeight["TapNoteScore_W1"]+
		getScoreTapNoteScore(score,"TapNoteScore_W2")*psWeight["TapNoteScore_W2"]+
		getScoreTapNoteScore(score,"TapNoteScore_W3")*psWeight["TapNoteScore_W3"]+
		getScoreTapNoteScore(score,"TapNoteScore_W4")*psWeight["TapNoteScore_W4"]+
		getScoreTapNoteScore(score,"TapNoteScore_W5")*psWeight["TapNoteScore_W5"]+
		getScoreTapNoteScore(score,"TapNoteScore_Miss")*psWeight["TapNoteScore_Miss"]+
		getScoreTapNoteScore(score,"TapNoteScore_CheckpointHit")*psWeight["TapNoteScore_CheckpointHit"]+
		getScoreTapNoteScore(score,"TapNoteScore_CheckpointMiss")*psWeight["TapNoteScore_CheckpointMiss"]+
		getScoreTapNoteScore(score,"TapNoteScore_HitMine")*psWeight["TapNoteScore_HitMine"]+
		getScoreTapNoteScore(score,"TapNoteScore_AvoidMine")*psWeight["TapNoteScore_AvoidMine"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_LetGo")*psWeight["HoldNoteScore_LetGo"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_Held")*psWeight["HoldNoteScore_Held"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_MissedHold")*psWeight["HoldNoteScore_MissedHold"]
	elseif scoreType == 3 then
		return
		getScoreTapNoteScore(score,"TapNoteScore_W1")*migsWeight["TapNoteScore_W1"]+
		getScoreTapNoteScore(score,"TapNoteScore_W2")*migsWeight["TapNoteScore_W2"]+
		getScoreTapNoteScore(score,"TapNoteScore_W3")*migsWeight["TapNoteScore_W3"]+
		getScoreTapNoteScore(score,"TapNoteScore_W4")*migsWeight["TapNoteScore_W4"]+
		getScoreTapNoteScore(score,"TapNoteScore_W5")*migsWeight["TapNoteScore_W5"]+
		getScoreTapNoteScore(score,"TapNoteScore_Miss")*migsWeight["TapNoteScore_Miss"]+
		getScoreTapNoteScore(score,"TapNoteScore_CheckpointHit")*migsWeight["TapNoteScore_CheckpointHit"]+
		getScoreTapNoteScore(score,"TapNoteScore_CheckpointMiss")*migsWeight["TapNoteScore_CheckpointMiss"]+
		getScoreTapNoteScore(score,"TapNoteScore_HitMine")*migsWeight["TapNoteScore_HitMine"]+
		getScoreTapNoteScore(score,"TapNoteScore_AvoidMine")*migsWeight["TapNoteScore_AvoidMine"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_LetGo")*migsWeight["HoldNoteScore_LetGo"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_Held")*migsWeight["HoldNoteScore_Held"]+
		getScoreHoldNoteScore(score,"HoldNoteScore_MissedHold")*migsWeight["HoldNoteScore_MissedHold"]
	else
		return 0
	end
end

------------------------------------------------
local function GetWifeScoreOrConvertFromDP(score)
	local o = score:GetWifeScore()
	if o > 0 then return o end
	return score:ConvertDpToWife()
end

-- Rate filter stuff -- 
local function scoreComparator(scoreA,scoreB)
	local a = GetWifeScoreOrConvertFromDP(scoreA)
	local b = GetWifeScoreOrConvertFromDP(scoreB)
	if a == b then 
		return scoreA:GetWifeScore() > scoreB:GetWifeScore()
	end
	return a > b 
end

function SortScores(hsTable)
	table.sort(hsTable,scoreComparator)
	return hsTable
end

-- returns a string corresponding to the rate mod used in the highscore.
function getRate(score)
	-- gets the rate mod used in highscore. doesn't work if ratemod has a different name
	local mods = score:GetModifiers()
	if string.find(mods,"Haste") ~= nil then
		return 'Haste'
	elseif string.find(mods,"xMusic") == nil then
		return '1.0x'
	else
		return (string.match(mods,"%d+%.%d+xMusic")):sub(1,-6)
	end
end

-- returns the index of the highscore in a given highscore table. 
function getHighScoreIndex(hsTable,score)
	for k,v in ipairs(hsTable) do
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
	if not o then return nil end
	
	for k,v in pairs(o) do
		o[k] = o[k]:GetScores()
	end
	
	return o
end

function getUsedRates(rtTable)
	local rates = {}
	local initIndex = 1 
	if rtTable ~= nil then
		for k,v in pairs(rtTable) do
			rates[#rates+1] = k
		end
		table.sort(rates,function(a,b) a=a:gsub("x","") b=b:gsub("x","") return a<b end)
		for i=1,#rates do
			if rates[i] == "1.0x" or rates[i] == "All" then
				initIndex = i
			end
		end
	end
	return rates,initIndex
end

function getScoreList(pn)
	local song = GAMESTATE:GetCurrentSong()
	local profile
	local steps
	if GAMESTATE:IsPlayerEnabled(pn) then
		profile = GetPlayerOrMachineProfile(pn)
		steps = GAMESTATE:GetCurrentSteps(pn)
		if profile ~= nil and steps ~= nil and song ~= nil then
			return profile:GetHighScoreList(song,steps):GetHighScores()
		end
	end
	return nil
end