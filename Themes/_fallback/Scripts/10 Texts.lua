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
	Grade_Tier01 = THEME:GetString("Grade", "Tier01"), -- AAAAA
	Grade_Tier02 = THEME:GetString("Grade", "Tier02"), -- AAAA:
	Grade_Tier03 = THEME:GetString("Grade", "Tier03"), -- AAAA.
	Grade_Tier04 = THEME:GetString("Grade", "Tier04"), -- AAAA
	Grade_Tier05 = THEME:GetString("Grade", "Tier05"), -- AAA:
	Grade_Tier06 = THEME:GetString("Grade", "Tier06"), -- AAA.
	Grade_Tier07 = THEME:GetString("Grade", "Tier07"), -- AAA
	Grade_Tier08 = THEME:GetString("Grade", "Tier08"), -- AA:
	Grade_Tier09 = THEME:GetString("Grade", "Tier09"), -- AA.
	Grade_Tier10 = THEME:GetString("Grade", "Tier10"), -- AA
	Grade_Tier11 = THEME:GetString("Grade", "Tier11"), -- A:
	Grade_Tier12 = THEME:GetString("Grade", "Tier12"), -- A.
	Grade_Tier13 = THEME:GetString("Grade", "Tier13"), -- A
	Grade_Tier14 = THEME:GetString("Grade", "Tier14"), -- B
	Grade_Tier15 = THEME:GetString("Grade", "Tier15"), -- C
	Grade_Tier16 = THEME:GetString("Grade", "Tier16"),
	Grade_Failed = THEME:GetString("Grade", "Failed")
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