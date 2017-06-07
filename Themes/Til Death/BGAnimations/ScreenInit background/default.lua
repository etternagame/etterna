local t = Def.ActorFrame {}

local minanyms = {
	-- the logorrhea of yore
	"Irate Platypusaurusean",
	"Pancreatic_MilkTrombone",
	"Fire_Elevator",
	"Starchy DarkButter",
	"Unememorabelia Bedelia",
	"Cheezits 'N Rice",
	"Scalding brain fart",
	
	-- the profile names of yore
	"mystic memer",
	"orange hands",
	"Sir Smauggy",
	"ScroogeMcdoot",
	"The King In The Hall Under The Mountain Amidst The Dragon's Lair",
	"Just Mash",
	"Just 5mash",
	"Player 1",
	"Minametra",
	"Noodlesim",
	"Default Profile",
	"Mina (backup)",
	"Mina",
	"orange hands (backup)",
	"Don Eon",
	"Tromwelskintherintherin",
	"StraitStrix",
	"UmbralChord",
	"NoSaucierMagic",
	"EgomaniaCircus",
	"Stepmania Bakery Hero",

	-- the nightly builds of yore
	"AVAST YE STEPMATEY",
	"ALPHA DINGOBABY",
	"TOWEL FOR A PHOENIX",
	"MAYBE ITS RASPBELLINE",
	"FREE MARKOV",
	"BARK ON MOONSTRING",
	"CARAMEL CANDELABRABELLUM",
	"PATENTED TOILET MYTHOS",
	"MOONAR LANDING - THE RETURWN",
	"ORANGE BLOSSOM SPECIAL",
	"SERENADE UNDER PORCUPINE",
	"PURPLE PARKING METERSTICK",
	"SERRATED HAMBURGER",
	"INTERGALACTIC TURKEY",
	"BICYCLE LAUGH",
	"GODLY PLATE OF THE WHALE"
}

math.random()

t[#t+1] = Def.ActorFrame {
  InitCommand=cmd(Center),
	LoadActor("woop") .. {
		OnCommand=cmd(zoomto,SCREEN_WIDTH,150;diffusealpha,0;linear,1;diffusealpha,1;sleep,1.75;linear,2;diffusealpha,0)
	},
	Def.ActorFrame {
	  OnCommand=cmd(playcommandonchildren,"ChildrenOn"),
	  ChildrenOnCommand=cmd(diffusealpha,0;sleep,0.5;linear,0.5;diffusealpha,1),
		LoadFont("Common Normal") .. {
			Text=getThemeName(),
			InitCommand=cmd(y,-24),
			OnCommand=cmd(sleep,1;linear,3;diffuse,getDifficultyColor("Difficulty_Couple");diffusealpha,0)
		},
		LoadFont("Common Normal") .. {
			Text="Created by " .. minanyms[math.random(#minanyms)],
			InitCommand=cmd(y,16;zoom,0.75),
			OnCommand=cmd(sleep,1;linear,3;diffuse,getDifficultyColor("Difficulty_Couple");diffusealpha,0)
		},
	}
}

return t
