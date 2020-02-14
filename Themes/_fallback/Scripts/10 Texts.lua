local judges = { "marv", "perf", "great", "good", "boo", "miss" }
local defaultJudges = {'TapNoteScore_W1','TapNoteScore_W2','TapNoteScore_W3','TapNoteScore_W4','TapNoteScore_W5','TapNoteScore_Miss'}

local shortDiffName = {
	Difficulty_Beginner	= 'BG',
	Difficulty_Easy		= 'EZ',
	Difficulty_Medium	= 'NM',
	Difficulty_Hard		= 'HD',
	Difficulty_Challenge= 'IN',
	Difficulty_Edit 	= 'ED',
	Beginner			= 'BG',
	Easy				= 'EZ',
	Normal				= 'NM',
	Hard				= 'HD',
	Insane 				= 'IN',
	Edit 				= 'ED',
}

local scoreTypeText = {
	"DP",
	"PS",
	"MIGS",
	"Wife",
	"Waifu",
}

local DiffName = {
	Difficulty_Beginner	= 'Beginner',
	Difficulty_Easy		= 'Easy',
	Difficulty_Medium	= 'Normal',
	Difficulty_Hard		= 'Hard',
	Difficulty_Challenge= 'Insane',
	Difficulty_Edit 	= 'Edit',
}

local gradeString = {
	Grade_Tier01 = "AAAAA", -- AAAAA
	Grade_Tier02 = "AAAA:", -- AAAA:
	Grade_Tier03 = "AAAA.", -- AAAA.
	Grade_Tier04 = "AAAA", -- AAAA
	Grade_Tier05 = "AAA:", -- AAA:
	Grade_Tier06 = "AAA.", -- AAA.
	Grade_Tier07 = "AAA", -- AAA
	Grade_Tier08 = "AA:", -- AA:
	Grade_Tier09 = "AA.", -- AA.
	Grade_Tier10 = "AA", -- AA
	Grade_Tier11 = "A:", -- A:
	Grade_Tier12 = "A.", -- A.
	Grade_Tier13 = "A", -- A
	Grade_Tier14 = "B", -- B
	Grade_Tier15 = "C", -- C
	Grade_Tier16 = "D",
	Grade_Failed = 'F'
}

local shortJudgeString = { -- Text strings for each Judgment types
	TapNoteScore_W1 = 'MA',
	TapNoteScore_W2	= 'PR',
	TapNoteScore_W3	 = 'GR',
	TapNoteScore_W4	= 'GD',
	TapNoteScore_W5	= 'BD',
	TapNoteScore_Miss = 'MS',			
	HoldNoteScore_Held = 'OK',	
	HoldNoteScore_LetGo = 'NG',	
}

local judgeString = { -- Text strings for each Judgment types
	TapNoteScore_W1 = 'Marvelous',
	TapNoteScore_W2	= 'Perfect',
	TapNoteScore_W3	 = 'Great',
	TapNoteScore_W4	= 'Good',
	TapNoteScore_W5	= 'Bad',
	TapNoteScore_Miss = 'Miss',			
	HoldNoteScore_Held = 'OK',	
	HoldNoteScore_LetGo = 'NG',	
}

function getShortDifficulty(diff)
	if diff ~= nil and diff ~= "" then
		return shortDiffName[diff]
	else
		return "ED"
	end
end;

function getDifficulty(diff)
	if diff ~= nil and diff ~= "" then
		return DiffName[diff]
	elseif diff == "" then
		return "Edit"
	else 
		return diff
	end
end;

function getGradeStrings(diff)
	if diff ~= nil then
		return gradeString[diff] or diff
	end
end;

function getJudgeStrings(judge)
	if judge ~= nil then
		return judgeString[judge] or judge
	end
end;

function getShortJudgeStrings(judge)
	if judge ~= nil then
		return shortJudgeString[judge] or judge
	end
end;

function getCustomJudgeString(customNames, judge)
	if judge ~= nil then
		return customNames and customNames[judges[judge]] or getJudgeStrings(defaultJudges[judge])
	end
end;

function scoringToText(st)
	if st == 0 then
		return scoreTypeText[themeConfig:get_data().global.DefaultScoreType]
	else
		return scoreTypeText[st]
	end
end;