-- **Stuff to do with point tables**
local CalculatedPointTableJudge					-- So we know whether or not our point tables need to be updated

-- Only need to run on standard scoring systems (1:3) can write something to automate checking scoring methods later
function updatePointTables()
	for i=1, 3 do 
		buildPointTable(scoringTypes[i]) 
	end
	CalculatedPointTableJudge = GetTimingDifficulty()
end

-- Initialize point tables and spit the judge value calculated under
function initPointTables()
	for i=1, #scoringTypes do
		buildPointTable(scoringTypes[i])
	end
	CalculatedPointTableJudge = GetTimingDifficulty()
end

function isJudgeSame()
	return CalculatedPointTableJudge == GetTimingDifficulty()
end

function isScoreJudgeSame()						-- For recalcing scores if judge values change, finish later
	return 1 -- CalculatedPointTableJudge == GetTimingDifficulty()
end

-- For a given scoring type output a table for which k = msoffset and v = pointvalue (only for timed judgments)
-- This is pretty inefficient however it's only done once per game load/judge switch
function buildPointTable(st)
	local timedWindows = {22.5,45,90,135,180}		-- kind of annoying but for now this'll be the solution to adjusting the eval display to account for judge value
	local function StandardPointTables(pt)			-- Wrapper for the existing scoring systems
		local function judgeIt(x)					-- Since we'll be storing ms values we can now recalc the same score on different judge values
			for i=1, #timedWindows do				-- not sure if there's a less obnoxious way to do this in lua
				if x <= timedWindows[i] then		-- this is like, really bad
					return st[ms.timedJudgments[i]]					
				end
			end
		end
		for i=0, timedWindows[5] do				-- Build the point table, beginning at index 0 for 0ms out to 180 for now
			pt[i] = judgeIt(i) 
		end		
		return pt
	end
	local function msScoringPointTables(pt)		-- Don't technically need a wrapper for this however we can accomodate basic variants easily so we might as well
		local function WifesCurves(x) 			-- should move to something based in normal distribution later	
			return  notShit.floor(st.maxPointValue - (st.linPenaltyFac * math.pow((x - st.curveBegin)/
				(st.curveEnd - st.curveBegin), st.expPenaltyFac)), 2) 	-- little bit of a mouthful, explanation at ~
		end																-- also should maybe use rond now that i have it
		for i=0, st.curveBegin-1 do 			-- first the "free zone", or, any values for which max points are automatically assigned
			pt[i] = st.maxPointValue 
		end			
		for i=st.curveBegin,st.curveEnd do		-- then curve out to the minimum points, or max penalty (same thing)
			pt[i] = WifesCurves(i)
		end		
		return pt
	end
	local function adjustTimingWindows(ts)		-- Adjust timing windows for the curent judge value (timing scale)
		for i=1, #timedWindows do 				-- We'll be modifying the local that gets reconstructed each time
			timedWindows[i] = timedWindows[i] * ts 
		end
	end

	adjustTimingWindows(PREFSMAN:GetPreference("TimingWindowScale"))
	if st.isMSBased == 1 then					-- Initialize the point table based on scoring method
		msScoringPointTables(st.PointTable)		-- make this more elegant later
	else
		StandardPointTables(st.PointTable)
	end
end