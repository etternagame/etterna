one of the main places from where i've been getting flak from is the etterna community, mostly for insisting that parts of the game are broken and that perhaps mina might not be as intelligent as first seems.

i don't make any real attempt to defend myself anywhere, but here's my rationale.

etterna in and of itself is far superior to its source material for keyboard play, there's no disputing the general improvements and tidying of the codebase; that said, the codebase is still a complete mess - only some 30% of the codebase is ever being used in general play.

so with that said i only consider it fair to criticise etterna for what it has added to the sm5 codebase. i can boil down the three selling points of etterna to be wife%, msd and a hashing algorithm; all three of these have flawed or incorrect implementations.

-- hashing

i'll start with the hashing algorithm because it's the simplest to explain. the point of a hashing algorithm is to assign a unique identifier to each file in the game so they can be referenced consistently in the game; game time as a file should always generate hash X, regardless of its location, similarly, game time should generate a different hash if the file is modified in any way.

however, 'in any way' is not particularly logical in this case, as changing the name of game time to anything else doesnt change how the file is played, so it should generate the same hash and therefore share the same score memory etc.

from that we can get that the hashing alg only needs to concern itself with the notes in the .sm, and not any of its metadata beyond BPM.

somehow, mina fucked this up.
basically, he ignores all of the non-relevant 0's in the .sm file (rows where are there are no notes) in an attempt to homogenize the awful .sm format, but the thing is, the .sm file formate uses these non-relevant 0's to determine note location in relevance to other notes

in essence, his hashing algorithm only checks that the notes are in the same order; it doesn't care at all about the location of the notes, so changing a 16th stream to a 32nd stream will generate the same hash, changing it to a poly will generate the same hash, changing it to an impossible 512th wall will generate the same hash, and slowing it down to a 4th stream will generate the same hash. all it checks is that the order of the notes is the same, and the bpm is the same.

also, this isn't fixable, there is no implicit way to translate existing hashes to a new format which accounts for the note location, without having some sort of "hash translator" which presumes that nobody has caused a hash collision at all (i have).

tl;dr replicable hash collisions

-- MSD

msd is just a pile of bandaid fixes over bandaid fixes.
firstly, the 6 categories msd splits difficulty into overlap and also don't fill the entire range of patterns. where would you put [14][23] splittrills in those 6 categories? jumpstream? but if i was to offset one hand by 1ms so that it was now 1423 (but effectively a splittrill) could you still class that as jumpstream despite having no jumps?

stream is calculated by checking repeated single notes and dividing if the repeated notes are 1234 or 4321
jumpstream is calculated by dividing the total jumps in the chart by the total notes
handstream is calculated by dividing the total hands in the chart by the total notes
this, obviously, means that you if you separate the hands from a handstream, and have the stream on one end of the file and a chordjack-wall of hands on on end of the file that it will class itself as handstream. you can test this by putting trickstarz edit and tld in the same file, it classes to handstream because the hands in tld mess with the ratio of hands:nothands in trickstarz. 
reference file: https://cdn.discordapp.com/attachments/523205845492957188/523210816330268714/tld_trickstarzedit.zip

there's some other factors involved but fundamentally this is how jumpstream/handstream are determined

also, a flaw in how factors are calculated is that they're subject to "impurity issues". for example, putting a runningman in a stream will increase the tech rating of that pattern, but will decrease the stream rating. if the increase in tech rating doesnt increase over the decreased stream rating, the pattern will go down in difficulty, despite being harder

this is most obvious in To Dimension, because of the lack of any technicality in the jumpstream, it actually wraps around and is determined to be the purest form of jumpstream, and therefore to dimension actually becomes the highest rated jumpstream file for its bpm.

to fairly prove this, we can take the file of to dimension and flip columns 2 and 3, which will turn all jumpstreams to [13][24] and generally replace 2h trills with far more difficult 1h trills and, as expected, the rating decreases because the jumpstream is now "less pure jumpstream".
this could be construed with a flaw in how overall difficulty is calculated (i believe it just takes the largest one of the 6 difficulty segments), but ultimately it's a fundamental mistake.

also, msd only acts in explicits, as in, it takes jumps as only being two notes at the same time, if you are to offset a note in a jump by 1ms, it will re-determine it as an extremely fast stream, instead of a graced jump note.
to fairly prove this, we can take a chordjack chart (say, tld), and offset all the notes by some negligable ms to remove all chords, surely enough, the file jumps up 3-4 points and loses 15 chordjack rating (becoming a 30-31 rated stream chart)
this means that any time a grace note is used in a file, MSD is rating the file incorrectly, and thinks that the grace note is an extremely fast stream (hence the huge increase in stream rating from unsnapping chords in TLD).

reference file: https://cdn.discordapp.com/attachments/420341018609582082/513540945879236608/the_lost_dedicated_snap.rar
(Note: The actual file I generated to test this is not offset by 1-2ms, it is from -3ms to +15ms; this is because the stepmania file format is complete trash, and Etterna cannot render BPMs above 1000 without teleporting notes in; also, for whatever reason, some jumps have remained. That said, the file plays entirely the same.)

As more practical proof of this, lets take a 'real' file that has a large amount of grace notes -- Angel Dust 2016 from 'd -- and re-snap them all.

https://sigurugis.files.wordpress.com/2018/11/withoutsnapfuckery.png
https://sigurugis.files.wordpress.com/2018/11/withsnapfuckery.png

Surely enough, the rating changes. But what is more notable is that the file with the jumps re-snapped (the normal difficulty) is now rated 2 points lower in 'stream', and also significantly lower in technical. This is effectively because the grace note'd jumps were treated as streams, and the technical disparity is likely* due to the fact that the sudden increase in speed presumed by the grace notes is classed as 'technical', or treated as polyrhythmic patterns of sort. Ultimately, the file plays the same, and only 20 or so grace notes were removed. reference file: https://cdn.discordapp.com/attachments/420341018609582082/513544086616276992/Angel_Dust_2016_DDMythical.rar

TL;DR: Using any sort of grace chord causes MSD to mis-rate and (in more extreme scenarios) mis-class a file. The innaccuracy increases as the usage of grace notes increase. The degree of change in MSD can be quite large (as seen in the TLD chart, which is increased in rating by 3 points, and shifted down in chordjacks by 15MSD).

also, MSD uses the length of a file in seconds (as calcuated by the position of the last note take away the position of the first) as a scaling factor for how hard a file is. This value does not increase indefinitely; files seem to stop scaling at 1:00, but I managed to get some files working in the 2:00 range. This means that file X, if lasting 50 seconds, will be rated differently than the file if it was 60 seconds long -- Regardless of the content in those additional 10 seconds. The 10 seconds can be entirely empty up until one final note at 1 minute, or it can contain a ridiculously impossible density of notes -- the point isn't the final result per se, but that the result is different on one multiplicative factor.

As an example, let's go into any file where the length is less than a minute (the effect is exacerbated the shorter the file is) and extend the file by adding a note at the minute mark, this will increase the length of the chart to 1 minute but ultimately, will not add any difficulty -- you should notice a sharp increase in MSD; e.g. Telephone shopping becomes a 30.68 (+5.11MSD), buckwild a 31.97 (+6.07MSD), and TV static a 32.60 (+6.19MSD). These aren't exactly minor increases, and I think it's pretty obvious that this one note alone doesn't actually increase the difficulty of the chart by 5-6MSD. In somewhat humorous essence, this allows you to effectively gain a large portion of MSD for AFKing. Therefore, using the length of a chart in seconds as a difficulty factor is not ideal, because it doesn't account for any notes actually in the section. I believe this factor was added under the presumption that no chart under a minute would have any significant portion of  'empty space', or filler.

As a tangential question -- What's the rate of filler that can be added which counteracts the increase in difficulty from the time factor? I'm assuming that the time factor caps at 1:00 here, but as stated above, sometimes it works beyond that. From testing on Harlem Shake, putting a note at the one minute margin, and then seeing how many notes i could put inbetween that note and the actual file, with notes that will add absolutely no difficulty to the chart and notes that are too far apart from eachother to cause any majorly discrepant inter-calculations. I got that you can put 159-160 notes in Harlem Shake after the 'real file' has ended, in the form of a 17.5bpm quadwall. That's an additional 24.84% of notes added to the file with negligable effect on the MSD (-0.02 at 160; but making it 159 notes keeps it at an MSD increase.) This isn't particularly conclusive, I don't know whether the time factor is linearly proportional to time, or exponentially proportional, but the point is that you can add almost a quarter of a files notes to itself in a non-difficulty-affective manner, and it will generate the same or effectively same MSD. I don't think it's difficult to understand that harlem shake with an additional 160 free notes is now an easier file -- it is -- and that the fact that these two files generate identical MSDs is indicative of a flaw in this method of calculation.

So what's the actual point of the time factor? Well, as far as I'm aware, it's to avoid short 'flukey' files from becoming ridiculously overrated -- i.e. players could grind a short file until they eventually "get lucky" and don't cbrush, and come out with a high MSD. This is a pretty clear band-aid fix to some sort of other issue in MSD (x pattern for 2 seconds is easier than x pattern for 1 minute), but let's assume it has this purpose and that it is a logically sound way of judging the 'flukability' of a file.

With it being time-based, and not note-based, this factor effectively states that the flukability of a file is directly proportional to the length of the file -- how often a player can play the file in a given timeframe -- and that longer files are less flukable inherently because the player has to spend more time on them. As shown above, using time as a factor has issues when the chart extends its length but changes its difficulty curve in that time period.

But, even if we were to take this factor as perfectly sound and perfectly working -- it isn't even implemented properly. Time Factor only takes the length of the chart on 1.0x. As proof of this, let's take Pokemon Battle from hi19 5, a ~1:30 stream file. Open some chart editing software and increase the BPM by 1.5x (to 275.610). Now compare this file's MSD to the rating of the original 1.0x file on 1.5x. These two files are identical, yet have different MSDs. The file that has had its BPM increased is rated 24.70, and the original file on 1.5x is 25.47. These files are identical, so why are they rated differently? It's because the time factor only takes the length of the file on 1.0x. The file with it's BPM increased is below 60 seconds on 1.0x; yet the original file is above 60 seconds on 1.0x. Despite the fact that both files have the same length when you rate the original by 1.5x, only the 1.0x length is used in difficulty calculations. You can prove this as true by going into the edited file and adding a note at the 1:30 mark. The files will now have identical MSD.

So not only is the actual concept of using the length of a file in seconds flawed because it doesn't take the actual file into account, but the actual implementation doesn't even work -- it only takes the files length on 1.0x.

This implies that any file that has been rated below 1 minute is rated incorrectly according to this factor's intent. Playing a file on 1.5x is NOT the same as editing the file to 1.5x speed. I honestly believe the prospect of using file length blindly to be antithetical to the point of a difficulty calculator -- what use is a factor if it doesn't even take the content or context of a chart into account?

I don't think anyone would argue that TV static is "really a 33". The issue is, actually, that MSD has no implicit stamina algorithm, or that, it doesn't have as much effect as it should have. The difference between a 400bpm stream pattern for 2 seconds vs the same pattern copypasted for 2 minutes is only about 2MSD, but I think it's agreeable that the gap is far larger than that (hence timefactor, which makes the difference between the two far larger at 8MSD.)

I believe the issue in-and-of-itself is that Stamina is not "really" a skillset on it's own; it requires another pattern to exist (Stamina-Jumpstream, etc.), but MSD treats it as if it can exist standalone -- Stamina, instead, should be torn out as an individual skillset, and applied implicitly to all of the other categories, so that there is no need to use the length of the chart in seconds as a difficulty factor.

This presumes that Stamina is being measured accurately/correctly by MSD, though.

also, half-rates are interpolated between the whole rate above and the whole rate below. i.e., the MSD for 1.15x is calculated by adding 1.1x and 1.2x's MSDs and then dividing by 2. This is an accuracy issue because it undermines the entire point of file scaling, it just presumes blindly that files increase linearly from half-rates. And, as a consequence, you can create identical files with different MSDs. Take any file, duplicate it, multiply the BPM by 1.05x, then compare it to the original 1.0x file on 1.05x; they will have different MSDs (unless by some miracle, they have the same MSD) but be exactly the same file. Therefore, half-rates are technically misrated, and all scores on them have generated incorrect MSDs -- interpolation for half-rates effectively undermines all of the nuance in a file when rating, and the only redeeming factor is that the difference between the real rating and the interpolated rating is generally pretty small. This is not a major issue, but any scenario where identical files do not generate identical MSDs (lack of self-similarity) shows flaws in the calculation mechanics.

When 2.0+ rates were added to the game, they simply extrapolated from the rating of 2.0x. The rating for a file on 2.n is the rating of it on (2.0's MSD / 2.0) x 2.n. This is the more egregious of the two, because if we take a file with just one note, the MSD of the chart will increase when rates exceed 2x, despite the fact that the file is still a single note in isolation -- identical chart, different MSDs. You can apply this to any chart by opening AV, doubling the bpm, then comparing the original chart on 2.1x to the new chart on 1.05x; they wont have the same MSD (unless by some miracle, they have the same MSD), but they will be the exact same file. This effectively implies that for every score above 2.0x rate, the MSD given for it is inaccurate.

tl;dr msd is just a series of bandaids that mina looked at and thought, "hm, that looks about right"; it will never converge to anything more accurate because it is fundamentally flawed and requires a full rewrite, but it will probably never get it.

-- Wife%

wife is a bad algorithm for determining how good a score is, not because it's ms based, but because it subsumed DP and all of its flaws. effectively, there is no serious punishment for bad MA or PA, all that matters is the cb count, and that is why players like sillyfangirl and mix can continue to get high level aa's with some 50-60% MA and 10% greats. this is why players keep getting "better" scores that have less wife% than their contemporaries, because wife% doesn't punish perfects or greats anywhere near as much as it should.

funnily enough, if you remove the hold reward from DP, and compare it to wife% on scores, you'll notice that the two are overwhelmingly similar (if not that wife is always some 0.09% less than DP, due to DP having no perfect penalty at all), you can check this with this script: https://hastebin.com/ecacuxoduf.py, throw your xml into it. (note this script was written in some 45 mins and is not very tidy)

wifepoints are calculated by: 2-10*(1-2^(-x^2/95^2))^2, where x is the ms.
you can view a desmos of this graph in action (where the output is in wife%, not wifepoints) here https://www.desmos.com/calculator/r25ctmwo9e

with this, we can solve for 1.86 (would-be-93%) and get x as ~40.5. the window for AA is 40.5ms, and the great window starts at 45ms. as you can probably infer, this means that MA is effectively irrelevant for aas, but somewhat relevant for scores in the 99.50+ range. as such, MA control is irrelevant for most players, or anyone not playing acc. you could consider this a good thing, but the byproduct is that mashing is easier.

similarly, this is *way* too lenient. everyone hits more marvelous than perfects, so you would actually want perfects to be *below* 93%, so that the ratio of players marv:perf will bring it up to an expected percent.

great punishment isn't particularly amazing either, with players generally getting around 89.65% for a 45ms great, 84.75% for a 50ms great and 78.50% for a 55ms great. These are absolutely miniscule punishments that would immediately be corrected by subsequent perfects/marvelous hits, so you'd require the player to hit an extremely high amount of these greats to actually have any risk of pulling them below the AA boundary (around 20%+ of your hits would have to be greats in this range to even come close to pulling you below an aa)

so with that you can get that, actually, all wife cares about is cbs; you can effectively hold your hand over the top three judgements of a score and just check the cbcount to see what wife is going to give it, because at most in regular play, MA and PA will only shift a score around by ~0.5% and in exceedingly mashed scores, 1-2%.

focusing entirely on cbs (and, by extent, cbrushing) is obviously not a good way of assessing someones ability to play a chart. cbs are important, sure, but the focus wife puts on them overshadows any amount of ma/pa a player has. this is ultimately why mashing is such an often-referenced topic with regards to d7/d8 play, because wife really doesn't do much to deter a player

-- but wait! it does!

cbrushes are the deterrent for mashing, and it obviously doesn't work, because mashing doesn't inherently result in a cbrush, it just increases the chance of you cbrushing. the thing is, in charts without any roll-traps or general technical fuckery (lay your hands on me comes to mind), you can mash with absolutely no risk of cbrushing, so these charts automatically fall out of wifes validity, because the punishment for mashing disappears. as such, using cbrushes to deter mashing is not a good idea, the implicit punishment, a "risk" of cbrushing, is not equivalent to an explicit punishment, i.e. punishing a player for hitting greats.

on a side note, mashing is not binary; if i was to have a chart of 100 mashable bursts, how many of them could i mash, and how many could i play legit, before the score in and of itself was not regarded as legitimate? one?, then we could divulge further and ask if jumpjacking a single 1212 makes the entire play illegitimate, or even the slightest degree of incorrect play could regard an entire score as illegitimate.

the obvious logical solution is to *PUNISH PA AND MA MORE, AND TO BALANCE THAT OUT, REDUCE THE FOCUS ON CBS; THE REDUCTION IN PUNISHMENT OF CBS*
but whatever.

tl;dr wife subsumes dp and therefore subsumes most of its issues, as dp was clearly designed for pad play and not keyboard play. it punishes mashing through cbrushes, and cbrushes alone, and therefore on charts where cbrushability is not very high, mashing is unpunished.

on a final note, i got a lot of flak for claiming that cbrushes are antithetical to the design of a rhythm game, i guess i should defend that here because it's somewhat relevant, but it's important to note that at absolutely no point did i suggest this to etterna, i suggested it to quaver, and deliberately did not suggest it to etterna because the entire game is designed around cbrushes.

me being against cbrushes isn't because i'm a "bad player" who "refuses to improve", it's because i think that it holds way too much value in the game. a cbrush of bads can effectively instantly end a score, or blitz it by some 2-3% if it isn't corrected quickly, as such, one of the main factors that determines your performance on a file is your ability to cancel cbrushes, which is done by simply not hitting a note. i consider it somewhat antithetical that the predominant deciding factor on how good your score is (i.e. end percent) is determined by your ability to not hit the notes, but that's digressable. in essence, you can't have a harsh punishment for a cb, because a cbrush will exacerbate that to an absurd level, and instantly end scores. but having a cb penalty that is low enough so that cbrushes have a moderate impact on scores will impact individual cbs, as you would effectively swurve any meaningful punishment for a single cb in a section.

underlying this is mostly this train of thought; if hitting a single cb is x punishment, and therefore x degree of bad, is a cbrush of y notes y*x as bad on the players part? i'd argue that the player isnt bleeding skill that fast for a single cbrush, and that's my underlying point. cbrushes harm the design of the game by forcing both a score% and difficulty calc to account for it, and that undermines the punishment of singular cb failures (or vice versa); what it effectively does is append an inherent difficulty to denser sections, regardless of difficulty, that there is more risk to them, for cbing in there is *far, far* more punitive than it would be on a section that is lighter. 

----
ok that's me defending all of my criticisms of Etterna's client as it exists now
i don't put much effort into defending my thought processes and as such people just construe whatever they want (https://sigurugis.files.wordpress.com/2018/10/dontlookatthefilename.png) from my points.

similarly, i write my thoughts down rather than putting them into a video/podcast form, and nobody seems to have the attention span to read things anymore. cool; i guess.

the only real issue that stems from all of these is the general overarching elitism from the etterna community - the instant vitriol quaver recieved for daring to exist is proof-positive of this; the game wasn't even out, nothing more than a one minute video showcasing a UI and a general note engine resulted in an overwhelming amount of unfounded speculative criticisms of the difficulty calc and stolen code (hilariously, spreading into its foundational elements - "C#, isn't that a bad language?"). The vitriol is unfounded when etterna in and of itself is ridiculously flawed

that said, I don't think much of Quaver, I'll probably have something written about my criticisms of that in a bit.

# Etterna

Etterna is an advanced cross-platform rhythm game focused on keyboard play.

| Mac                                   | Linux-clang                           | Linux-gcc                             | Windows 7                         | Windows 10                        | Coverity                            |
| ------------------------------------- | ------------------------------------- | ------------------------------------- | --------------------------------- | --------------------------------- | ----------------------------------- |
| [![build1-trav][]][build-link-travis] | [![build2-trav][]][build-link-travis] | [![build3-trav][]][build-link-travis] | [![build1-app][]][build-link-app] | [![build2-app][]][build-link-app] | [![build1-cov][]][build-link-cover] |

[build1-trav]: https://travis-matrix-badges.herokuapp.com/repos/etternagame/etterna/branches/develop/1
[build2-trav]: https://travis-matrix-badges.herokuapp.com/repos/etternagame/etterna/branches/develop/2
[build3-trav]: https://travis-matrix-badges.herokuapp.com/repos/etternagame/etterna/branches/develop/4
[build1-app]: https://appveyor-matrix-badges.herokuapp.com/repos/Nickito12/etterna/branch/develop/1
[build2-app]: https://appveyor-matrix-badges.herokuapp.com/repos/Nickito12/etterna/branch/develop/2
[build1-cov]: https://img.shields.io/coverity/scan/12978.svg
[build-link-travis]: https://travis-ci.org/etternagame/etterna
[build-link-app]: https://ci.appveyor.com/project/Nickito12/etterna
[build-link-cover]: https://scan.coverity.com/projects/etternagame-etterna

![Discord](https://img.shields.io/discord/339597420239519755.svg)

![Github Releases (by Release)](https://img.shields.io/github/downloads/etternagame/etterna/v0.60.0/total.svg)

## Installation

### MacOS

To install on Mac we currently require a few things to be installed.

First step is to run this in your terminal, this will install Homebrew.
```bash
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

The next step is to install the required libssl version using the following commands.
```bash
brew update && brew upgrade;
brew uninstall openssl;
brew install --force openssl@1.1;
sudo ln -s /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib /usr/local/lib/libcrypto.1.1.dylib;
sudo ln -s /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib /usr/local/lib/libssl.1.1.dylib;
```

The final step is to whitelist the Etterna directory, we cannot afford code signing and Apple forces it for the file system access we need to load NoteSkins and Songs.

```bash
sudo xattr -r -d com.apple.quarantine ~/your/path/to/Etterna
```

You path to Etterna is where ever you placed the folder inside of the DMG. If you copied it to your Desktop for example and renamed the folder, your path would be ``~/Desktop/Etterna``

### From Packages

For those that do not wish to compile the game on their own and use a binary right away, be aware of the following issues:

- Windows users are expected to have installed the [Microsoft Visual C++ x86 Redistributable for Visual Studio 2015](http://www.microsoft.com/en-us/download/details.aspx?id=48145) prior to running the game. For those on a 64-bit operating system, grab the x64 redistributable as well. [DirectX End-User Runtimes (June 2010)](http://www.microsoft.com/en-us/download/details.aspx?id=8109) is also required. Windows 7 is the minimum supported version.
- macOS users need to have macOS 10.6.8 or higher to run Etterna.
- Linux users should receive all they need from the package manager of their choice.

### From Source

https://etternagame.github.io/wiki/Building-Etterna.html

## Resources

- [Website](https://etternaonline.com/)
- [Discord](https://discord.gg/ZqpUjsJ)
- [Lua for Etterna](https://etternagame.github.io/Lua-For-Etterna/)
- [Lua API Reference](https://etternagame.github.io/Lua-For-Etterna/API/Lua.xml)
- [ETTP docs](https://github.com/Nickito12/NodeMultiEtt/blob/master/README.md)

## Licensing Terms

In short — you can do anything you like with the game (including sell products made with it), provided you _do not_ claim to have created the engine yourself or remove the credits.

For specific information/legalese:

- All of the our source code is under the [MIT license](http://opensource.org/licenses/MIT).
- The [MAD library](http://www.underbit.com/products/mad/) and [FFmpeg codecs](https://www.ffmpeg.org/) when built with our code use the [GPL license](http://www.gnu.org).

Etterna began as a fork of https://github.com/stepmania/stepmania

## Building

[On Building](Docs/Building.md)

## Building Documentation

[On Building Documentation](Docs/Building-Docs.md)

## Collaborating

[On Collaborating](Docs/Contributing.md)

## Bug Reporting

[On Bug Reporting](Docs/Bugreporting.md)
