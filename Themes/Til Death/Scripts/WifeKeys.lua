local curScoreKey

local difficultyconvert = {		-- need to double check these
	Beginner = "Novice",
	Easy = "Light",
	Medium = "Standard",
	Hard = "Heavy",
	Challenge = "Oni",
}

function getCurKey()
	return GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey()
end

function fashionChartKey(song, steps)
	local rv = 3
	local d = string.sub(steps:GetDifficulty(), 12)		-- not using that short string bullshit with 3 function calls of unnecessary overhead, gotta cut the fat where u can aite
	
	local s = song:GetSongFilePath()				
	s = string.gsub(s, "ssc$", "sm")					-- so the thing is .ssc files are virtually identical to .sm files in terms of their abhorrent information:space ratio however one of the few aspects 
	s = File.Read(s)									-- in which they are not identical happens to be the assumption of data organization that the below pattern is built on. What a surprise.
	s = string.match(s, "single[^;]+"..d..".-(\n[^\r\n][^\r\n][^\r\n][^\r\n]\r.-);") or string.match(s, "single[^;]+"..difficultyconvert[d]..".-(\n[^\r\n][^\r\n][^\r\n][^\r\n]\r.-);") -- man patterns are cool
	if not s then return nil end						-- so for now forget .ssc files, afaik every .ssc generates a .sm as well and we'll just read that instead. really it makes sense to just use a file format
														-- that isn't total garbage in the first place, but whatever
	local sb = notShit.round(song:GetFirstBeat())
	local eb = notShit.round(song:GetLastBeat())
	local o,l,b,t,k = {},{},{},{},{}
	local n
	local j
	
	if not string.find(s, ",") then								-- for those 1 meaure files...
		for y in string.gmatch(s, '\n(....)\r') do
			k[#k+1] = y
		end
		return "X"..SHA1StringHex(table.concat(k))
	end
	
	j = 0
	k = realsplit(s, ",")
	for i=1,#k do	
		t.y,t.l = {},{}
		n = 0
		if not string.find(k[i], '\n(....)\r') then break end	-- stop at the last measure that actually has notes
		for y in string.gmatch(k[i], '\n(....)\r') do			-- i -really- didnt want to have to do that 
			n = n + 1
			if y ~= "0000" then	
				t.y[#t.y+1] = y
				t.l[#t.l+1] = n
			end
		end
		for ix=1, #t.y do
			o[#o+1] = t.y[ix]
			l[#l+1] = t.l[ix] + j
		end
		for ix=1, n, n/4 do
			if intab(ix, t.l) then
				b[#b+1] = t.y[getvalkey(ix, t.l)]
			else
				b[#b+1] = "0000"
			end
		end
		j = j + n
	end

	l.s,l.e = {},{}
	for i=sb,eb do
		l.e[#l.e+1] = song:GetTimingData():GetElapsedTimeFromBeat(i+1)
		l.s[#l.s+1] = song:GetTimingData():GetElapsedTimeFromBeat(i)
	end
	
	for i=1, math.min(eb-sb-1,#b-sb) do
		o[#o+1] = b[i+sb]..":"..notShit.round(l.e[i]-l.s[i], rv)
	end

	--File.Write(song:GetSongDir().."keyrecord.txt", table.concat(o, "\n"))
	--File.Write(song:GetSongDir().."key.txt", "X"..SHA1StringHex(table.concat(o))) -- remove later
	return "X"..SHA1StringHex(table.concat(o))
end

function fashionScoreKey(score)
	curScoreKey = "S"..SHA1StringHex(score:GetDate())
	return curScoreKey
end

function getCurScoreKey()
	return curScoreKey
end