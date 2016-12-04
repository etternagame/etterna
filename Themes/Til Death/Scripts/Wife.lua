-- **Parameters for all the scoring types**
-- Aggregate these so we aren't writing 10 years worth of if statements in every function
-- global here
scoringTypes = { 
	{ -- Score Weights for DP score (MAX2)	vvvv is all this shit really necessary??? vvvv
		TapNoteScore_W1					= THEME:GetMetric("ScoreKeeperNormal","GradeWeightW1"),					--  2
		TapNoteScore_W2					= THEME:GetMetric("ScoreKeeperNormal","GradeWeightW2"),					--  2
		TapNoteScore_W3					= THEME:GetMetric("ScoreKeeperNormal","GradeWeightW3"),					--  1
		TapNoteScore_W4					= THEME:GetMetric("ScoreKeeperNormal","GradeWeightW4"),					--  0
		TapNoteScore_W5					= THEME:GetMetric("ScoreKeeperNormal","GradeWeightW5"),					-- -4
		TapNoteScore_Miss				= THEME:GetMetric("ScoreKeeperNormal","GradeWeightMiss"),				-- -8
		HoldNoteScore_Held				= THEME:GetMetric("ScoreKeeperNormal","GradeWeightHeld"),				--  6
		TapNoteScore_HitMine			= THEME:GetMetric("ScoreKeeperNormal","GradeWeightHitMine"),			-- -8
		HoldNoteScore_LetGo				= THEME:GetMetric("ScoreKeeperNormal","GradeWeightLetGo"),				--  0
		HoldNoteScore_MissedHold	 	= THEME:GetMetric("ScoreKeeperNormal","GradeWeightMissedHold"),
		TapNoteScore_AvoidMine			= 	0,
		TapNoteScore_CheckpointHit		= THEME:GetMetric("ScoreKeeperNormal","GradeWeightCheckpointHit"),		--  0
		TapNoteScore_CheckpointMiss 	= THEME:GetMetric("ScoreKeeperNormal","GradeWeightCheckpointMiss"),		--  0
		isMSBased						=   0,  -- Flag so the calc knows which point table function to use
		PointTable						=  {},  
},
	{ -- Score Weights for percentage scores (EX oni)
		TapNoteScore_W1					= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightW1"),
		TapNoteScore_W2					= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightW2"),
		TapNoteScore_W3					= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightW3"),
		TapNoteScore_W4					= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightW4"),
		TapNoteScore_W5					= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightW5"),
		TapNoteScore_Miss				= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightMiss"),
		HoldNoteScore_Held				= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightHeld"),
		TapNoteScore_HitMine			= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightHitMine"),
		HoldNoteScore_LetGo				= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightLetGo"),
		HoldNoteScore_MissedHold	 	= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightMissedHold"),
		TapNoteScore_AvoidMine			=   0,
		TapNoteScore_CheckpointHit		= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightCheckpointHit"),
		TapNoteScore_CheckpointMiss 	= THEME:GetMetric("ScoreKeeperNormal","PercentScoreWeightCheckpointMiss"),
		isMSBased						=   0,  -- Flag so the calc knows which point table function to use
		PointTable						=  {},  
},
	{ -- Score Weights for MIGS score (terrible scoring system)
		TapNoteScore_W1				=  3,
		TapNoteScore_W2				=  2,
		TapNoteScore_W3				=  1,
		TapNoteScore_W4				=  0,
		TapNoteScore_W5				= -4,
		TapNoteScore_Miss			= -8,
		HoldNoteScore_Held			=  6,
		TapNoteScore_HitMine		= -8,
		HoldNoteScore_LetGo			=  0,
		HoldNoteScore_MissedHold 	=  0,
		TapNoteScore_AvoidMine		=  0,
		TapNoteScore_CheckpointHit	=  2,
		TapNoteScore_CheckpointMiss	= -8,
		isMSBased					=  0,
		PointTable					= {},
	},
	{ -- Measurements for Wife 
		curveBegin					=  18,	-- Ms values below this point are assigned maxPointValue		
		curveEnd 					= 150,	-- Ms values above this point are assigned maxPenalty	
		linPenaltyFac	 			= 9.5,	-- Linear strength of the curve (can be thought of as max penalty, see below)
		expPenaltyFac 				=   2,	-- Exponential strength of the curve. Wow. Such 2, much convenient, very prime even number.
		maxPointValue 				=   2,	-- Maximum point value per tap, same as MAX DP for consistency
		maxPenalty 					=-7.5,	-- Maximum penalty, can also be though of as maxPointValue - linPenaltyFac (minimum points)
		TapNoteScore_W1				=   2,  -- An alias for maxPointValue, for compatability with the multiscoring wrapper
		TapNoteScore_Miss			=-7.5,  -- Non timed judgments are evaluated identically to the existing systems
		HoldNoteScore_Held			=   0,
		TapNoteScore_HitMine		=-7.5,
		HoldNoteScore_LetGo			=  -6,
		HoldNoteScore_MissedHold	=  -6,
		TapNoteScore_AvoidMine		=   0,
		TapNoteScore_CheckpointHit	=   0,
		TapNoteScore_CheckpointMiss =   0,
		isMSBased					=   1,  -- Flag so the calc knows which point table function to use
		PointTable					=  {},  
	},
	{ -- Measurements for Waifu
		curveBegin					=  36,	-- Ms values below this point are assigned maxPointValue		
		curveEnd 					= 180,	-- Ms values above this point are assigned maxPenalty	
		linPenaltyFac	 			= 9.5,	-- Linear strength of the curve (can be thought of as max penalty, see below)
		expPenaltyFac 				=   2,	-- Exponential strength of the curve. Wow. Such 2, much convenient, very prime even number.
		maxPointValue 				=   2,	-- Maximum point value per tap, same as MAX DP for consistency
		maxPenalty 					=-7.5,	-- Maximum penalty, can also be though of as maxPointValue - linPenaltyFac (minimum points)
		TapNoteScore_W1				=   2,  -- An alias for maxPointValue, for compatability with the multiscoring wrapper
		TapNoteScore_Miss			=-7.5,  -- Non timed judgments are evaluated identically to the existing systems
		HoldNoteScore_Held			=   0,
		TapNoteScore_HitMine		=-7.5,
		HoldNoteScore_LetGo			=  -6,
		HoldNoteScore_MissedHold	=  -6,
		TapNoteScore_AvoidMine		=   0,
		TapNoteScore_CheckpointHit	=   0,
		TapNoteScore_CheckpointMiss =   0,
		isMSBased					=   1,  -- Flag so the calc knows which point table function to use
		PointTable					=  {},  
	},
}

-- **stuff that needs to go into options later**
local fallbackscoreType = 1				-- this indicates the theme to search for and return scoringTypes[x] scores if no scoringTypes[default] scores can be found
function getfallbackscoreType()			
	return fallbackscoreType
end
-- end of stuff that needs to go into options later

-- returns a string of keys for a table
function showKeys(t)
	local o={}
	for k,v in pairs(t) do
		o[#o+1]=k
	end
	return table.concat(o, ",") -- apparently theres an alias for this in this game who knew
end

-- same but returns a table (array)
function tableKeys(t)
	local o={}
	for k,v in pairs(t) do
		o[#o+1]=k
	end
	return o
end

-- **Scoring functions**
-- Need to integer encode everything later when new system is constructed
-- edit: no i dont hahahahahhahahahaha it takes like 50mb on the disk in total, but this still should be reorganized
-- fairly certain it's simply unnecessary to have so many nested tables
function buildPlayerScore(newScore, score, td, nrd)		-- score == playerstagestats or a score loaded from the existing hs table
	local function fillMetadata(s)					-- td = deviance timing data, if nil we're converting from an existing score
		s.DateAchieved = score:GetDate()
		s.Grade = score:GetGrade()
		s.MaxCombo = score:GetMaxCombo()
		s.Mods = score:GetModifiers()
		s.Rate = getRate(score)						-- need to deal with this in a more rubust fashion eventually
		s.JudgeAchievedOn = 0						-- put this in only if score is non converted
		return(s)
	end	
	local function fillTimedJudgments(s, score, td)	-- These are all "taps"
		if not td then
			s.TimingData = "Converted"				-- No timing data means we're converting from a pre-existing score
			s.TimedJudgments = ms.convertJudgmentsToMs(score)		--- aaa need to redo this eventually
			s.JudgmentRow = "NA"
		else
			s.TimingData = ms.round(td,4)			-- moving the int round into the freq function so digits can be preserved for the original data set, out to 4 for now
			s.TimedJudgments = ms.freq(td)			-- also considering removing abs from the above process to preserve early/late info--//And then frequency them
			s.TimingData = table.concat(td, ",")	
			s.JudgmentRow = table.concat(nrd, ",")
		end
		s.TimedJudgments = ms.tableConcat(s.TimedJudgments) 	-- stringit, technically we don't need to but im testing the packer and unpacker
		return(s)												-- although there are probably libs that do this faster and without fail
	end
	local function fillNonTimedJudgments(s)
		for i=1,#ms.nonTimedTaps do			
			s[ms.nonTimedTaps[i]] = score:GetTapNoteScore(ms.nonTimedTaps[i])
		end
		for i=1,#ms.nonTimedHolds do			
			s[ms.nonTimedHolds[i]] = score:GetHoldNoteScore(ms.nonTimedHolds[i])
		end
		return(s)
	end
	local function fillWindowJudgments(s)						-- Original game judgments
		for i=1, #ms.timedJudgments do
			s[ms.timedJudgments[i]] = score:GetTapNoteScore(ms.timedJudgments[i])
		end
		return s
	end
	local function fillAllOldJudgments(s)						-- shorten this crap later
		for i=1,#TapNoteScore do			
			s[TapNoteScore[i]] = score:GetTapNoteScore(TapNoteScore[i])
		end
		for i=1,#HoldNoteScore do			
			s[HoldNoteScore[i]] = score:GetHoldNoteScore(HoldNoteScore[i])
		end
		return(s)
	end
	local function batchCalcScores(s, td)						-- Calculate score for each scoringType so we can save them all now and retrieve later
		for i=1,3 do											-- Standard scoring
			s[#s+1] = calcPlayerScore(newScore, i)
		end
		if td then												-- Only calculate ms based scores if we have timing data
			for i=4,#scoringTypes do							-- indexes got screwed up and ScoreTable[4] would return as nil despite existing in the file???
				s[#s+1] = calcPlayerScore(newScore, i)			-- Should probably make this more flexible
			end
		end
		return(s)
	end
	
	if newScore.Metadata ~= nil then
		return newScore
	end
	newScore = fillTimedJudgments(newScore, score, td)			-- Fill in the original offset data and the freq table
	newScore.Metadata = fillMetadata({})						-- General score metadata such as song title etc etc
	newScore.NonTimedJudgments = fillNonTimedJudgments({})		-- Misses/mines etc
	newScore.WindowJudgments = fillWindowJudgments({})			-- Might as well
	newScore.AllOldJudgments = fillAllOldJudgments({})			-- oh fuck it
	newScore.ScoreTable = batchCalcScores({}, td)				-- Will contain percent values for all scoring types as well as the judge value calc'd under
	--newScore.Msd = {}											-- Future stuff maybe
	if newScore.TimingData == "Converted" then 					-- can use the judgments to ms unpacker for recalcing later, don't need to save this
		newScore.TimedJudgments = "Converted"
	end
	return newScore
end

-- A single function to calculate score percentages for any system whoa
-- also super needs to be rewritten, whoa
function calcPlayerScore(score, scoreType)
	local st = scoringTypes[scoreType]			-- First grab the parameters table for whichever scoring system is being called
	local tj = ms.split(score.TimedJudgments)	-- Split the timed judgments string
	local s = {}
	local o = 0
	local ntj = score.NonTimedJudgments

	s.Percent = 0								-- actual % score
	s.Points = {}								-- various point values (player, total, etc)
	s.JudgeCalc = GetTimingDifficulty() 		-- judge value the score was calculated under, not necessary for ms based systems but whatever for now
	s.ParamsHash = 	SHA1StringHex(stringDatTable(st))	-- string hash for the scoring type parameters at the time of calculation//need to remember to 
														-- hash the function for ms scoring as well
	-- Player points first
	local pt = st.PointTable					-- Get the point table for the current scoring system
	for i=0, #tj do								-- Multiply the recorded judgments against their point weight
		o = o + (tj[i] * (pt[i] or pt[#pt]))	-- Anything for i>#pt is assigned pt[#pt], the assumption here is that 
	end											-- the final value of the curve is the same as the miss value (may not necessarily be true in the future)
												-- **note: this is like, HILARIOUSLY inefficient for converting old scores, by a factor of 300 or more*
												-- however this is a factor being applied to negligible calculation times
	s.Points.playerTimedPoints = o				-- Gotta make this score type specific eventually (prolly score.PointTable) //did this
	for i=1, #ms.nonTimedScored do				-- Now nontimed judgments (mines/misses/holds etc)
		o = o + st[ms.nonTimedScored[i]] * ntj[ms.nonTimedScored[i]]
	end
	s.Points.playerTotalPoints = o				
	
	-- Calculate the maximum attainable points for the given score/song and scoring system based on currently played (unused atm)
	local function calcMaxPointsFromPlayed()
		local taps = (st.TapNoteScore_W1 * (ms.tableSum(tj) + ntj.TapNoteScore_Miss))
		local holds = (st.HoldNoteScore_Held * (ntj.HoldNoteScore_Held + ntj.HoldNoteScore_MissedHold)) or 0
		return taps + holds
	end	
	s.Points.maxTapPoints = st.TapNoteScore_W1 * getMaxNotes(PLAYER_1)			-- need to remove dependency on getMaxNotes and getMaxHolds functions
	s.Points.maxHoldPoints = st.HoldNoteScore_Held * getMaxHolds(PLAYER_1)
	s.Points.maxTotalPoints = s.Points.maxTapPoints + s.Points.maxHoldPoints
	s.Percent = notShit.round(s.Points.playerTotalPoints/s.Points.maxTotalPoints*100, 2)
	if scoreType == 4 then
		s.Percent = dafinalscoreYO
	end
	return s
end

-- **Functions related to the storage and retrieval of chart data and score data**
-- whole section should be redone eventually
function formLink(x, y) x[y] = x[y] or {} return x[y] end
local curChart
local cutScore
function msTableChartUpdate(song, steps)
	local function msTableChartInit(song, steps)		
		local function fillChartMeta(c)
			c.SongTitle = song:GetDisplayMainTitle()
			c.Duration = notShit.floor(song:GetLastSecond() - song:GetFirstSecond(), 2)
			c.StepsDifficulty = steps:GetDifficulty()
			c.RadarValues = {}
			for i=1,#ms.RelevantRadars do 				-- Grabbing the relevant radar values i.e not that voltage garbage no one has cared about for almost 2 decades
				c.RadarValues[ms.RelevantRadars[i]] = steps:GetRadarValues(PLAYER_1):GetValue(ms.RelevantRadars[i])
			end
			return c
		end
		local function fillMsdData(c)
			c.MsdDifficulty = ""
			c.PrimaryFileType = ""
			c.SecondaryFileType = ""
			c.StaminaIntensity = ""
			return c
		end
		local function fillFluxRecord(c)
			c.OriginalBpmLine = ""
			c.RedundanciesRemoved = ""
			c.MsOffsets = ""
			return c
		end
		
		local c = {}
		c.ChartMeta = fillChartMeta({})					-- General inforomatienne
		--c.Msd = fillMsdData({})						-- Later stuff possibly
		--c.FluxRecord = fillFluxRecord({})				-- Later stuff possibly
		c.Scores = {}									-- Scores housed here for now and by rate, too, wow
		c.PlayerOptions = {}							-- for AlwaysPlayMirror later
		return(c)
	end

	local chartKey = getCurKey()
	local Chart
	
	if not doesCurChartTableExist() then 
		Chart = msTableChartInit(song, steps)	
	else
		Chart = getCurChartByKey(chartKey)
	end
	
	local needsave = convertOldHighScoreList(Chart.Scores)
	curChart = Chart
	if needsave == true then
		saveCurChart()
	end
end

function saveCurChart()
	if getCurKey() ~= "Invalid" then
		File.Write("/wife/"..getCurKey()..".lua",DataDumperSlow(getCurChart()))
	end
end

-- should probably find an alternative not involving loadstring since apparently its the most expensive option literally in the universe
function load_conf_file(fname)
	local file= RageFileUtil.CreateRageFile()
	local ret= {}
	if file:Open(fname, 1) then
		local data= loadstring(file:Read())
		setfenv(data, {})
		local success, data_ret= pcall(data)
		if success then
			ret= data_ret
		end
		file:Close()
	end
	file:destroy()
	return ret
end

function doesCurChartTableExist()
	return FILEMAN:DoesFileExist("/wife/"..getCurKey()..".lua")
end

function getCurChartByKey()							-- Should split chart info and score info at some point
	return load_conf_file("/wife/"..getCurKey()..".lua")
end

function getCurChart()								-- Should split chart info and score info at some point
	return curChart
end

function convertOldHighScoreList(Scores)			-- should have a better system that checks whether or not scores need to be converted
	local o = false
	local oldScores = getScoreList(PLAYER_1)
	for i=1, #oldScores do
		local oldscore = oldScores[i]							
		local RateScores = formLink(Scores, getRate(oldscore))
		local scoreKey = fashionScoreKey(oldscore)					
		if not RateScores[scoreKey] then
			o = true
			local newScore = formLink(RateScores, scoreKey)			-- newScore.ScoreTable == Chart.Scores[getRate(oldscore)][fashionScoreKey(oldscore)].ScoreTable)
			newScore = buildPlayerScore(newScore, oldscore, nil)	-- nil == converted score
		end
	end
	return o
end

function getCurScore()
	return curScore
end

function setCurScore(score)
	curScore = score
end

function getCurRateString()
  local rate = GAMESTATE:GetSongOptions('ModsLevel_Song')
  if rate == "" then
    rate = "1.0x"
  elseif rate == "2.0xMusic" then
	rate = "2.0x"
  end
  rate = string.gsub(rate,"Music","")
  return rate
end

function getCurRateScores()
	return getCurChart().Scores[getCurRateString()]
end