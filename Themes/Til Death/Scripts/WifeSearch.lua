-- turns out its a million times easier to do this internally wow who knew

-- local allSongs
-- local allTitles = {}
-- local allArtists = {}
-- local songSearchResult = {nil, nil}
-- local prevResultSong
	

-- function changePreviewMusic(song)						-- jesus christ did i really have to do this, like, you guys couldn't just have built this into the setsong function?????
	-- SOUND:StopMusic()
	-- SOUND:PlayMusicPart(song:GetPreviewMusicPath(),song:GetSampleStart(),song:GetSampleLength(),1,0.3,true,true,true)
	-- prevResultSong = song
-- end

-- function setSongFromSearch(resultSong)					-- should probably move more of the stuff in songsearch.lua here since this stuff is here anyway
	-- if resultSong then									-- im really starting to not like this 
		-- for i=1, 2 do
			-- SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(resultSong)
		-- end

		-- GAMESTATE:SetCurrentSong(resultSong)
		-- GAMESTATE:SetCurrentSteps(PLAYER_1, resultSong:GetAllSteps()[1])
		-- if not prevResultSong then 
			-- changePreviewMusic(resultSong)
		-- elseif prevResultSong ~= resultSong and prevResultSong:GetMainTitle() ~= resultSong:GetMainTitle() then		-- probably irritating if you're switching between a few results for the same song trying to figure out which version you want to play 
			-- changePreviewMusic(resultSong)																			-- and the music keeps resetting, granted, this isn't going to catch the same songs with slightly different titles but oh well
		-- end																											-- maybe disable this if i find people identify which chart they want to play by the preview music point
	-- end
	-- MESSAGEMAN:Broadcast("RefreshSearchResults")
-- end

-- function setSongFromCompletedSearch()
	-- for i=1, 2 do
		-- SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(songSearchResult[1])
	-- end
	-- GAMESTATE:SetCurrentSong(songSearchResult[1])
	-- GAMESTATE:SetCurrentSteps(PLAYER_1, songSearchResult[2])
	-- local MusicWheel = SCREENMAN:GetTopScreen():GetChild("MusicWheel");
	-- MusicWheel:Move(-1);
	-- MusicWheel:Move(1);
	-- MusicWheel:Move(0);
-- end

-- function storeSongSearchResult(song, steps)		-- either a song and steps object if the search was concluded through start or nil if the search was cancelled through back
	-- songSearchResult = {song, steps}
-- end

-- function recallSearchResult()
	-- return songSearchResult[1]
-- end

-- function storedSearchInfo()
	-- return allSongs
-- end

-- function storeSearchInfo(songs)
	-- allSongs = songs
	-- for i=1, #allSongs do
		-- allTitles[i] = allSongs[i]:GetMainTitle():lower()
		-- allArtists[i] = allSongs[i]:GetDisplayArtist():lower()	
	-- end
-- end

-- function getAllSongs()	
	-- return allSongs
-- end

-- function getAllTitles()	
	-- return allTitles
-- end

-- function getAllArtists()
	-- return allArtists
-- end