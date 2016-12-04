-- **Big thing that contains all sorts of fun stuff related to ms based scoring and adapting existing systems to it**
-- probably need to reorganize large parts of this
-- global here
ms = {}

-- *Values and stuff*
-- Make the distinction between ms timed and non-timed judgments
ms.timedJudgments = {
	"TapNoteScore_W1", 
	"TapNoteScore_W2", 
	"TapNoteScore_W3", 
	"TapNoteScore_W4", 
	"TapNoteScore_W5",
}

-- J4 assumed, used for conversion
ms.timedWindows = {
	22.5,
	45,
	90,
	135,
	180,
}

-- Non timed notes and holds
ms.nonTimedTaps = {
	"TapNoteScore_Miss",
	"TapNoteScore_HitMine",		
	"TapNoteScore_AvoidMine",
	"TapNoteScore_None",
	"TapNoteScore_CheckpointMiss",
	"TapNoteScore_CheckpointHit",
}
ms.nonTimedHolds = {
	"HoldNoteScore_Held",
	"HoldNoteScore_MissedHold",			
	"HoldNoteScore_LetGo",		
	"HoldNoteScore_None",
}

-- Nontimed judgments that are scored
ms.nonTimedScored ={
	"TapNoteScore_Miss",
	"TapNoteScore_HitMine",
	"HoldNoteScore_Held",
	"HoldNoteScore_MissedHold",	
	"HoldNoteScore_LetGo",
}

-- Radar values people on earth actually care about
ms.RelevantRadarsShort = {
	"Notes:",
	"Jumps:",
	"Hands:",
	"Holds:",
	"Mines:",
	"TotalTaps:",
	"Rolls:",
	"Lifts:",
	"Fakes:",
}

ms.RelevantRadars = {
	"RadarCategory_Notes",
	"RadarCategory_Jumps",
	"RadarCategory_Hands",
	"RadarCategory_Holds",
	"RadarCategory_Mines",
	"RadarCategory_TapsAndHolds",
	"RadarCategory_Rolls",
	"RadarCategory_Lifts",
	"RadarCategory_Fakes",
}

-- lists just for displayed judgment counts
ms.JudgeCount = {
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo",
}

ms.JudgeCountInverse = {
	TapNoteScore_W1 		= 1,
	TapNoteScore_W2 		= 2,
	TapNoteScore_W3 		= 3,
	TapNoteScore_W4 		= 4,
	TapNoteScore_W5 		= 5,
	TapNoteScore_Miss		= 6,
	HoldNoteScore_Held	 	= 7,
	HoldNoteScore_LetGo		= 8,
}

-- **Functions**
function ms.ok(m)
	if not m then
		SCREENMAN:SystemMessage("nahbro")
	else
		SCREENMAN:SystemMessage(m)
	end
end

function ms.type(m)
	SCREENMAN:SystemMessage(type(m))
end

-- Calculates ms frequency of the rounded timing data
function ms.freq(td)
	local c
	local o = ms.freqTableInit(200)			-- oops....
	for i=1,#td do 							-- moved the rounding to integer values here so digits can be preserved in the original data
		c = notShit.round(math.abs(td[i]))
		o[c] = o[c] + 1
	end
	return o
end

-- Reset tables that begin at 0 rather than 1
function ms.tableReset(t)
	local o = t
	for i=0, #t do 
		o[i] = 0 
	end 
	return o
end

-- Wrapper for full timing data tables
function ms.round(t, y)
	for i=1,#t do
		t[i] = notShit.round(t[i], y)
	end
	return(t)
end

-- **pretty sure like 90% of this is unnecessary**
-- Same as table.concat but doesnt blow up because there's a 0 index
function ms.tableConcat(t)				-- need to deal with this better later
	local z = t[0]
	t[0] = nil
	local o = z..","..table.concat(t, ",")
	t[0] = z
	return o
end

-- Unpack ms frequency strings where the index must begin at 0
function ms.split(s)
	local o = {}
	local function split(i,j)
		j = string.find(s,",",j+1)
		if not j then 
			o[#o+1] = string.sub(s,i+1)
			return o
		end
		o[#o+1] = string.sub(s,i+1,j-1)
		return split(j,j)
	end
	local j = string.find(s,",")
	o[0] = string.sub(s,1,j-1)
	return split(j,j)
end

-- Init frequency tables to n, or the max ms to be tracked (for now it will be 180)
function ms.freqTableInit(n)
	local o = {}
	for i=0,n do  
		o[i] = 0 
	end 
	return o
end

-- Table sum designed for ms tables (optional starting position, default to 0, and absolute value flag)
function ms.tableSum(t, n, abs)
	local o = 0
	for i=n or 0, #t do
		if abs then 
			o = o + math.abs(t[i])
		else o = o + t[i] end
	end
	return o
end

function wifeMean(t)
	local c = #t
	if c == 0 then return 0 end
	local o = 0
	for i=1,c do 
		if t[i] ~= 1000 then
			o = o + t[i]
		end
	end
	return o/c
end

function wifeStd(t)
	local u = wifeMean(t)
	local u2 = 0
	for i=1,#t do
		if t[i] ~= 1000 then
			u2 = u2 + (t[i]-u)^2
		end
	end
	return math.sqrt(u2/(#t-1))
end

function wifeRange(t)
	local x,y = 10000,0
	for i=1,#t do
		if t[i] ~=1000 then
			if math.abs(t[i]) < math.abs(x) then x = t[i] end
			if math.abs(t[i]) > math.abs(y) then y = t[i] end
		end
	end
	return x,y
end

-- for converting old scores
function ms.convertJudgmentsToMs(score)
	local timedWindows = {22.5,45,90,135,180}		-- k this is getting opague to the point of disaster... the conversion process assumes everything is j4 however the point tables are calculated
	local function adjustTimingWindows(ts)		-- on current judge which means we need to assume it by using the current point tables, either that or a j4 conversion table is initiated and 
		for i=1, #timedWindows do 					-- permanent (this is probably the best solution but w.e), another solution is just to write a general function for conversion that isn't garbage
			timedWindows[i] = timedWindows[i] * ts 	-- like all of the existing ones....
		end
	end
	adjustTimingWindows(PREFSMAN:GetPreference("TimingWindowScale"))											
	local o = ms.freqTableInit(200)					-- Only grab timed judgments (marv-boo)
	for i=1, #ms.timedJudgments do 					-- Since we don't know what judge level the scores were achieved on we have to assume j4
		o[math.floor(timedWindows[i])] = getScoreTapNoteScore(score,ms.timedJudgments[i])	-- Assign them to the max integer ms value for their respective windows
	end
	return o
end

-- **Very slight efficiency rewrites**
notShit = {}
function notShit.floor(x, y)
	y = 10^(y or 0)
	return math.floor(x*y)/y
end

function notShit.ceil(x, y)
	y = 10^(y or 0)
	return math.ceil(x*y)/y
end

-- seriously what is math and how does it work
function notShit.round(x, y)
	y = 10^(y or 0)
	return math.floor(x*y+0.5)/y
end

-- more bullshit, pretty sure theres something that does this already but its faster to write it than find it (only used for hashing scoring prefs)
function stringDatTable(t)
	local o = {}
	for k,v in pairs(t) do 
		if type(v) ~= "table" then	-- should probably move the point tables to a separate object so i dont need to do this
			o[#o+1] = k..v
		end
	end
	return table.concat(o)
end

-- old msd rate thing
-- Grabs the currently selected rate as a string in the form of "r.rx"
function getCurRateString()
	return string.format("%.1f",getCurRateValue()).."x"
end

function getCurRateDisplayString()
	return string.format("%.1f",getCurRateValue()).."xMusic"
end

function getCurRateValue()
  return notShit.round(GAMESTATE:GetSongOptionsObject('ModsLevel_Current'):MusicRate(),3)
end