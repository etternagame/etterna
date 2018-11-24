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

These are Commands which are usable for all actors, regardless of screen.

### InitCommand

Executed before the screen displays it's 'on' state. Useful to initialize actor state (Like position, sizes, storing the reference to the actor in a file-local variable)

### OnCommand

Executed as the screen is displayed (After all InitCommands). Useful, for example, to begin animations as the player enters a screen.

### OffCommand

Executed as the screen is being exited.

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

# Screen Specific

Note: In case something is missing in any particular screen, you can check the ScreenX.cpp files under /src/ in the github repository, checking what is broadcasted by MESSAGEMAN.

## ScreenSelectMusic

### SetMessageCommand

Broadcast when a `MusicWheelItem`' is being set with new information, such as when scrolling up and down. Gets the `Song` currently selected in the `MusicWheel`

    params = {
        Song = An instance of the `Song` that was just set to the `MusicWheelItem`,
        Index = The index of the `MusicWheelItem` that was just set,
        HasFocus = If the `MusicWheelItem` is focused or not,
        Text = The name of the song group this `MusicWheelItem` is from,
    }

### CurrentStepsP1ChangedMessageCommand

Triggered when the currently selected `Steps` (a.k.a chart within a `Song`) change, whether it be by changing the difficulty or selecting another song.

### CurrentSongChangedMessageCommand

Triggered when the `MusicWheel` current `Song` changes.

### PreviousSongMessageCommand or NextSongMessageCommand

Triggered when the player selects a different `Song` in the `MusicWheel` by tapping left or right.

### ChangeStepsMessageCommand

Triggered when `Steps` are changed. Need to check player & direction using ChangeStepsMessageCommand=function(self, params) then params.Player and params.Direction.

    params = {
        Player =PLAYER_1 or PLAYER_2,
        Direction = 1 or -1,
    }

## ScreenGameplay

### LifeChangedMessageCommand

Activated whenever a player's life changes.

| Parameters | Description                             |
| ---------- | --------------------------------------- |
| Player     | Either PLAYER_1 or PLAYER_2             |
| LifeMeter  | Amount of life in a decimal from 0 to 1 |

If the lifebar is type is battery it will also have LivesLeft and LostLife.

### HealthStateChangedMessageCommand

Activated whenever a player's health state changes...

| Parameters     | Description                                                                                                              |
| -------------- | ------------------------------------------------------------------------------------------------------------------------ |
| PlayerNumber   | Either PLAYER_1 or PLAYER_2                                                                                              |
| HealthState    | A HealthState Enum, which is either `HealthState_Hot`, `HealthState_Alive`, `HealthState_Danger`, or `HealthState_Dead`. |
| OldHealthState | self explanatory.                                                                                                        |

### PlayerFailedMessageCommand

This one's obvious.

| Parameters   | Description                 |
| ------------ | --------------------------- |
| PlayerNumber | Either PLAYER_1 or PLAYER_2 |

### ScoreChangedMessageCommand

Activated whenever a player's score changes. Params include PlayerNumber and MultiPlayer, but can also include ToastyCombo in certain cases.

| Parameters   | Description                 |
| ------------ | --------------------------- |
| PlayerNumber | Either PLAYER_1 or PLAYER_2 |
| MultiPlayer  | ???                         |

### JudgmentMessageCommand

Triggered when a judgment happens, either because a player stepped on a note or they completely missed it.

| Parameters    | Description                      |
| ------------- | -------------------------------- |
| Player        | Either PLAYER_1 or PLAYER_2      |
| MultiPlayer   | If they're multiplayer, probably |
| TapNoteSccre  | The TapNoteScore                 |
| Early         | True if early, false if late     |
| TapNoteOffset | Offset of the judgement          |
| HoldNoteScore | The HoldNoteScore                |

### ComboChangedMessageCommand

Activated whenever a combo changes.

| Parameters       | Description                                                      |
| ---------------- | ---------------------------------------------------------------- |
| Player           | Either PLAYER_1 or PLAYER_2                                      |
| OldCombo         | ???                                                              |
| OldMissCombo     | ???                                                              |
| PlayerState      | An instance of PlayerState. This may not always be present.      |
| PlayerStageStats | An instance of PlayerStageStats. This may not always be present. |

### ToastyAchievedMessageCommand

| Parameters   | Description                 |
| ------------ | --------------------------- |
| PlayerNumber | Either PLAYER_1 or PLAYER_2 |
| ToastyCombo  | ???                         |
| Level        | ???                         |

### ToastyDroppedMessageCommand

| Parameters   | Description                 |
| ------------ | --------------------------- |
| PlayerNumber | Either PLAYER_1 or PLAYER_2 |

### DoneLoadingNextSongMessageCommand

Unknown, might be triggered during course mode

## ActorScroller specific

### GainFocusCommand

Triggered when the ActorScroller is selected

### LoseFocusCommand

Triggered when the ActorScroller is deselected

## Downloads

###
