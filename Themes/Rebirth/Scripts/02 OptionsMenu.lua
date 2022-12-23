function getPlayerOptionsList(itemSet)
	local Items = {
		["Main"] = "Speed,RateList,NoteSk,RS,PRAC,CG,ScrollDir,Center,Persp,LC,BG,SF,Background,Judge,Life,Fail,Assist,Score",
		["Theme"] = "CBHL,JT,JA,CT,CL,DP,TT,TG,TTM,JC,EB,EBC,PI,FB,MB,LEADB,NPS",
		["Effect"] = "Persp,App,GHO,SHO,Acc,Hide,Effect1,Effect2,Scroll,Turn,Insert,R1,R2,Holds,Mines"
	}
	return Items[itemSet] .. ",NextScr"
end
