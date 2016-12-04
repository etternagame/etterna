-- should be merged with other stuff later and need to test to make sure it //actually// does nothing if scores aren't 4k

storeSongSearchResult(nil)

local score
local Scores
local RateScores
local newScore

if not getCurKey() then 					-- these are probably fairly redundant checks but oh well for now
	ms.ok("No chart key detected (probably never generated)... skipping wife calculations...")
else
	if STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1):GetPlayedSteps()[1]:GetStepsType() == "StepsType_Dance_Single" then
		score = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1):GetHighScore()
		Scores = getCurChart().Scores
		RateScores = formLink(Scores, getRate(score))
		newScore = formLink(RateScores, fashionScoreKey(score))
		newScore = buildPlayerScore(newScore, score, devianceTable, NoteRowTable)
		setCurScore(newScore)
		saveCurChart()
		SCREENMAN:SystemMessage("Wife: Score Saved")
	else
		SCREENMAN:SystemMessage("Wife: Not 4k singles, Stop wasting my time")
	end
end
return