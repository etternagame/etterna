#Actor Commands

## Command

A command is basically an `Actor` function defined by lua. These can be called instantly using playcommand(commandStringName) or queued for the next frame using queuecommand(commandStringName) on an actor. Usually, they identify certain events that happen, and are called accordingly. Parameters to Commands are '(self, params)' with params being an optional table of named parameters. Note that the command's name must be stripped of the suffix 'Command' when playing/queueing it.

    Def.ActorFrame {
        InitCommand = function(self)
            self:x(5)
        end
    }

## MessageCommand

A 'message' command is registered as XMessageCommand (Instead of XCommand), and the registered command will still be called X, but it will be flagged as a message command. This means that if someone broadcasts a message (Using `MSGMAN` a.k.a message manager) it will trigger/execute that command (Meaning, that function). Parameters to MessageCommands are '(self, params)' with params being an optional table of named parameters.

## Universal Commands

These are Commands which are usable for all actors, regardless of screen and type of actor.

### InitCommand

Executed before the screen displays it's 'on' state. Useful to initialize actor state (Like position, sizes, storing the reference to the actor in a file-local variable)

### OnCommand

Executed as the screen is displayed (After all InitCommands). Useful, for example, to begin animations as the player enters a screen.

### OffCommand

Executed as the screen is being exited.

### SetMessageCommand

Broadcast the information the actor is meant to display changes. This is usually used for some specific types of actors and for actors defined in some specific /Graphics/ files. TODO: Document them all

    params = {???}

### AnimationFinishedCommand

Executed when an animation is finished (If one was executed on this actor)

### CodeMessageCommand

Executed when any button specified in metrics is pressed. You must have CodeNames set in the respective screen in metrics.ini for this to function correctly.

    params = {
        PlayerNumber = PLAYER_1 or PLAYER_2 ,
        Name = the name of the code you specified. So if you have `Codeleft="Left"` in metrics.ini and you press left, params.Name would be "left"
    }

Example metrics:

    CodeNames="ResetJudge,PrevJudge,NextJudge,ToggleHands"
    CodeResetJudge="MenuUp"
    CodeNextJudge="EffectUp"
    CodePrevJudge="EffectDown"
    CodeToggleHands="MenuDown"

### DFRStartedMessageCommand

Broadcast when starting a Differential Song Reload.

    params = { }

### DFRFinishedMessageCommand

Broadcast when finishing a Differential Song Reload.

    params = {
    	newsongs = number
    }

### DFRUpdateMessageCommand

Broadcast during Differential Song Reload to update the loading text lua.

    params = {
    	txt = string,-- Something like "Loading:\nPackName\nSongName"
    }

### SystemMessageMessageCommand

Broadcast when a SystemMessage is sent.

    params = {
    	Message = string,
    	NoAnimate = bool
    }

### HideSystemMessageMessageCommand

Broadcast when the currently displayed SystemMessage is hidden (Stops displaying).

    params = { }

### RefreshCreditTextMessageCommand

Broadcast when the credits text is refreshed

    params = { }

### ScreenChangedMessageCommand

Broadcast when the current Screen changes.

    params = { }

### NewPlaylistMessageCommmand

Broadcast when a playlist is created. Sends a `Playlist`

    params = {
    	newplaylist = playlist
    }

### CrossedBeatMessageCommand

Broadcast when playing sound from a Song/Chart and a beat passes.

    params = {
    	Beat = number(integer)
    }

### LeftClickMessageCommand

Broadcast when left mouse button is clicked

    params = { }

### RightClickMessageCommand

Broadcast when right mouse button is clicked

    params = { }

### MiddleClickMessageCommand

Broadcast when middle mouse button is clicked

    params = { }

### WheelDownMessageCommand

Broadcast when the mouse wheel wheel is scrolled down

    params = { }

### WheelUpMessageCommand

Broadcast when the mouse wheel wheel is scrolled up

    params = { }

### MenuUpP1MessageCommand

Broadcast when the MenuUp button gets input.

    params = { }

### MenuLeftP1MessageCommand

Broadcast when the MenuLeft button gets input.

    params = { }

### MenuRightP1MessageCommand

Broadcast when the MenuRight button gets input.

    params = { }

### MenuDownP1MessageCommand

Broadcast when the MenuDown button gets input.

    params = { }

### DLProgressAndQueueUpdateMessageCommand

Broadcast when the pack download user interface needs to be updated.

    params = {
    	dlsize:number of packs being downloaded right now,
    	dlprogress: string with the status of current downloads,
    	queuesize: number of packs in the queue (Not downloading at the moment),
    	queuedpacks: string with the names of queued packs
    }

### GoalTableRefreshMessageCommand

Broadcast when goals change (Added, removed or changed). See GetPlayerOrMachineProfile(PLAYER_1):GetGoalTable() `Profile`

    params = { }

### XPreferenceChangedMessageCommand

Broadcast when preference X changes (Like AdditionalSongsFolderPreferenceChangedMessageCommand)

    params = { }

# Screen Specific

Note: In case something is missing in any particular screen, you can check the ScreenX.cpp files under /src/ in the github repository, checking what is broadcast by MESSAGEMAN.

## ScreenSelectMusic

Note: All of this should apply in ScreenNetSelectMusic too

### RouletteStoppedMessageCommand

Broadcast when the roulette is used, after it stops.

    params = { }

### FilterResultsMessageCommand

Broadcast rebuilding the music wheel. This usually happens at screen creation and whenever a search or filter is performed.

    params = {
        Total = number, -- total songs
        Matches = number, -- number of songs that fall within the search/filter criteria
    }

### DelayedChartUpdateMessageCommand

Broadcast after a small delay of not scrolling in the music wheel. Should be used to update "heavy" objects that we dont want to update while scrolling.

    params = { }

    			Message msg("FavoritesUpdated");
    			MESSAGEMAN->Broadcast(msg);
    	MESSAGEMAN->Broadcast("PlayingSampleMusic");

### RateChangedMessageCommand

Broadcast when the current rate changes (So things like the current chart difficulty and song length can be updated).

    params = { }

### CurrentStepsP1ChangedMessageCommand

Triggered when the currently selected `Steps` (a.k.a chart within a `Song`) change, whether it be by changing the difficulty or selecting another song.

### CurrentSongChangedMessageCommand

Triggered when the `MusicWheel` current `Song` changes.

### PreviousSongMessageCommand or NextSongMessageCommand

Triggered when the player selects a different `Song` in the `MusicWheel` by tapping left or right.

### ChangeStepsMessageCommand

Triggered when `Steps` are changed. Need to check player & direction using ChangeStepsMessageCommand=function(self, params) then params.Player and params.Direction.

    params = {
        Player =PLAYER_1,
        Direction = 1 or -1,
    }

## ScreenGameplay

### ToastyAchievedMessageCommand

Broadcast when a toasty is achieved

    params = {
    	PlayerNumber = PLAYER_1,
    	ToastyCombo = number,
    	Level = number
    }

### LifeChangedMessageCommand

Broadcast globally whenever a player's life changes.

    params = {
        Player =PLAYER_1,
        LifeMeter = Amount of life in a decimal from 0 to 1,
    }

### HealthStateChangedMessageCommand

Activated whenever a player's health state changes...

    params = {
    	PlayerNumber = PLAYER_1,
    	HealthState = A HealthState Enum, which is either `HealthState_Hot`, `HealthState_Alive`, `HealthState_Danger`, or `HealthState_Dead`.,
    	OldHeathState= same as above, but old one
    }

### PlayerFailedMessageCommand

This one's obvious.

    params = {
    	PlayerNumber = PLAYER_1
    }

### JudgmentMessageCommand

Triggered when a judgment happens, either because a player stepped on a note or they completely missed it.

    params = {
        Player = PLAYER_1,
    	TapNoteScore= string,
    	HoldNoteScore = string or nil, -- only present for holds
    	NoteRow = number or nil,-- only present for taps
    	TapNoteOffset = decimal number or nil, -- only present for taps. In miliseconds
    	Offset = decimal number or nil, -- only present for non miss taps. TapNoteOffset*1000
    	Early = bool, -- only present for taps. TapNoteOffset > 0
    	FirstTrack=number, -- track/column of the judgment if CCoff, otherwise first column/track
    	Judgment=string, -- one of 'TapNoteScore_W1' to W5 or 'TapNoteScore_Miss'
    	Type="Tap" or "Mine" or "Hold",
    	CurWifeScore=decimal number, -- current score at this point in the chart
    	MaxWifeScore=decimal number, -- max possible score at this point in the chart
    	WifeDifferential= decimal number, -- distance/differential to the target score (NOT percent)
    	TotalPercent=number between -500 and 100, -- 100 * curwifescore / totalwifescore
    	WifePercent=number between -500 and 100, -- 100 * curwifescore / maxwifescore
    	-- These 2 PB ones are nil if pb%=targetgoal%
    	WifePBDifferential=nil or decimal number, -- curwifescore - maxwifescore * wifescorepersonalbest
    	WifePBGoal=nil or decimal number between -500 and 100, -- percentage
    	NumTracks = number or nil, -- nil for mines/taps
    	Val = number or nil, -- nil for mines. Unknown usage
    	TapNote = TapNote or nil, -- not nil for holds. Unknown usage
    	MultiPlayer = number, -- unknown usage
    	Holds = {HoldHead}, -- Only present for taps. Held holds table
    	Taps= {TapNote}, -- Only present for taps
    }

### ScoreNoneMessageCommand

Broadcast globally whenever a TapScoreNone is scored (I'm not sure when that is :/ )

    params = { }

### ComboChangedMessageCommand

Broadcast globally whenever combo changes.

    params = {
        Player =PLAYER_1,
    	OldCombo = number (integer),
    	OldMissCombo = number (integer),
    	PlayerState = nil or PlayerState,
    	m_pPlayerStageStats = nil or m_pPlayerStageStats,
    }

### ToastyDroppedMessageCommand

Broadcast when the toasty combo is broken (Non perfect/marvelous is scored)

    params = {
    	PlayerNumber = PLAYER_1
    }

### DoneLoadingNextSongMessageCommand

Unknown, might be triggered during course mode

    params = { }

## ActorScroller specific

### GainFocusCommand

Triggered when the ActorScroller is selected

    params = { }

### LoseFocusCommand

Triggered when the ActorScroller is deselected

    params = { }

## Downloads

### PausingDownloadsMessageCommand

Broadcast when we're pausing all downloads because of gameplay screen. Should hide or minimize the pack downloader UI.

    params = { }

### ResumingDownloadsMessageCommand

Broadcast when we're unpausing all downloads because of leaving gameplay screen. Should show or maximize the pack downloader UI.

    params = { }

### AllDownloadsCompletedMessageCommand

Broadcast when no downloads are left.

    params = { }

### DownloadFailedMessageCommand

Broadcast when a pack download succeeds. See `DownloadablePack`

    params = {
    	pack = DownloadablePack
    }

### DownloadFailedMessageCommand

Broadcast when a pack download fails. See `DownloadablePack`

    params = {
    	pack = DownloadablePack
    }

## Profile Tracker

The profile tracker is used with DLMAN(`DownloadManager`), For example EO (https://etternaonline.com/). The game sends the tracker scores, goals and favourites once connected, and it allows usage of the ingame leaderboard.

### LoginMessageCommand

Broadcast when succesfully logging in to score tracker. See DLMAN:Logout(),DLMAN:Login(user, pass) and DLMAN:LoginWithToken(user, passToken) in `DownloadManager`

    params = { }

### LoginFailedMessageCommand

Broadcast when a pack download succeeds. See DLMAN:Logout(),DLMAN:Login(user, pass) and DLMAN:LoginWithToken(user, passToken) in `DownloadManager`

    params = { }

### LogOutMessageCommand

Broadcast when logging out of the tracker. See DLMAN:Logout(),DLMAN:Login(user, pass) and DLMAN:LoginWithToken(user, passToken) in `DownloadManager`

    params = { }

### FavouritesUpdateMessageCommand

Broadcast when favourites are retrieved/refreshed from the tracker.

    params = { }

### OnlineUpdateMessageCommand

Broadcast when our top25/rank/ratings/country are updated from the tracker. See DLMAN:GetTopSkillsetScore(n, skillsetString),DLMAN:GetSkillsetRating(), DLMAN:GetUsername(), DLMAN:GetSkillsetRank(skillsetString) in `DownloadManager`

    params = { }

## Multiplayer

### MultiplayerDisconnectionMessageCommand

Broadcast when multiplayer connection ends.

    params = { }

### MultiplayerConnectionMessageCommand

Broadcast when multiplayer connection starts.

    params = { }

### NewMultiScoreMessageCommand

Broadcast when we recieve a score from the server. See NSMAN:GetEvalScores() `NetworkSyncManager`

    params = { }

### ChatMessageCommand

Broadcast when we recieve a chat message from the server.

    params = {
    	tab = number,
    	msg = string -- Contains "|crrggbba" to identify color changes
    }

### MPLeaderboardUpdateMessageCommand

Broadcast when we recieve an update during gameplay on the state of other players playing with us (So, if we're diplaying a leaderboard in a theme, we should update it with the new values). See NSMAN:GetMPLeaderboard() `NetworkSyncManager`

    params = { }

### MultiplayerDisconnectionMessageCommand

Broadcast when multiplayer connection ends.

    params = { }

### ChartRequestMessageCommand

Broadcast when we recieve a new ChartRequest from the server. See NSMAN:GetChartRequests() `NetworkSyncManager`

    params = { }

### ChartRequestMessageCommand

Broadcast when we recieve a new ChartRequest from the server. See NSMAN:GetChartRequests() `NetworkSyncManager`

    params = { }

### UsersUpdateMessageCommand

Broadcast when we the userlist needs to be refreshed (Both room and lobby userlists). See NSMAN:GetLobbyUserList() `NetworkSyncManager` and GetUserQty,GetUser,GetUserReady,GetUserState in `ScreenNetSelectMusic`

    params = { }

todo/missing: Messages in OptionList.cpp, StepMania.cpp, "ScoreChanged", "UpdateNetEvalStats", "ScoreNone", ScreenOptions.cpp, "CancelAllP1", ScreenSelectMaster.cpp,, ScreenSelectMusic.cpp,
