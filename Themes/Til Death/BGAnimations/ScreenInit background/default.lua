local t = Def.ActorFrame {}

local minanyms = {
	"the logorrhea of yore",
	"Irate Platypusaurusean",
	"Pancreatic_MilkTrombone",
	"Fire_Elevator",
	"Starchy DarkButter",
	"Unememorabelia Bedelia",
	"Cheezits 'N Rice",
	"Scalding brain fart",
	"Luridescence",
	"Frothy Loin",
	"Ministry of Silly Steps",
	"\na haiku for man\ntried to prove that we're special\nturns out that we're not",
	"BinkleBompFOUR[emdash]",
	"an astounding lack of self awareness (Mina 1:78)",
	"the changelog that answers your question\nyou know the one you didn't read",
	"the profile names of yore",
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
	"stepmania bakery hero",
	"infinite swirling squid spacecraft",
	"Jack Can't Reacher",
	"the nightly builds of yore",
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
	"SERRATED HAMBURGER",
	"INTERGALACTIC TURKEY",
	"TURTLE LOVE",
	"BICYCLE LAUGH",
	"GODLY PLATE OF THE WHALE",
	"PURPLE PARKING METERSTICK",
	"BROCCOLI MOISTURIZER",
	"SLIGHTLY ALTERED COMPILER FLAG",
	"PARTIALLY Q-TIP KIWI",
	"POTATO PAINTING COURTESY",
	"Ye olde names",
	"Shoeeater9000",
	"Thirdeye",
	"Otiose Velleity",
	"MoreLikeYourMomesis",
	"RofflesTheCat",
	"MinaEnnui",
	"FishnaciousGrace",
	"Forp",
	"Forp II",
	"Forp III The Unavenged",
	"Lady Mericicelourne Ciestrianna De'anstrasvazanne",
	"Caecita",
	"Tempestress",
	"unself",
	"DefinitelyNotMina",
	"KillerClown",
	"Quirky Colonel Kibbles",
	"2c",
	"FroggerNanny",
	"FroggyNanner",
	"Aeristacicianistriaza",
	"FroggerNanny The Unfrogged",
	"ScatPlayKatarina",
	"Ferric Chloride Matter",
	"Bananatiger",
	"Unapologetically Hostile Entity",
	"Perpetual Sarcasm Dispensary",
	"dehydrated mandelbrot pulsar",
	"desires a pink anime avatar",
	"theamishwillneverseethis.jpg",
	"Restore missing legacy Stepmania Team credits #1588",
	"gratuitous double negative usage",
	"MinaTallerThanBrandon",
	"confers monetary value to words",
	"notcool",
	"mina restepped as a pad file",
	"sapient typo conglomerate",
	"Real Stepmania Client",
	"Paraplebsis",
	"Qlwpdrt ~!- V~!@#B",
	"HypophoraticProcatalepsis",
	"WobblyChickenRepeat",
	"RoundTableTigerSwan",
	"SkeleTotemWalkRedux",
	"TinkleTotemJamboree",
	"LerpNurbs",
	"HerpingDerper",
	"MinaHatesYouYesYou",
	"ImaginaryStepmaniaClient",
	"ExtraLunarTangoFoxtrot",
	"Morbid Papaya Matrix",
	"note pink",
	"borp",
	"stringofcharactersyouwillonlyseeifitsindexisselectedbytheprngfunctionusedtochoosefromthetableitshousedin",
	"b&",
	"oxford semicolon",
	"more bugs added than fixed",
	"mr.takesallthecreditdoesnoneofthework",
	"Inappropriately placed political message that prompts angry responses on the internet",
	"phalangeography platform for moonlight vigil",
}

math.random()

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(0, 0):halign(0):valign(0):zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(color("#111111")):diffusealpha(0):linear(
			1
		):diffusealpha(1):sleep(1.75):linear(2):diffusealpha(0)
	end
}

t[#t + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:Center()
	end,
	LeftClickMessageCommand = function(self)
		SCREENMAN:GetTopScreen():StartTransitioningScreen("SM_GoToNextScreen")
	end,
	LoadActor("woop") ..
		{
			OnCommand = function(self)
				self:zoomto(SCREEN_WIDTH, 150):diffusealpha(0):linear(1):diffusealpha(1):sleep(1.75):linear(2):diffusealpha(0)
			end
		},
	Def.ActorFrame {
		OnCommand = function(self)
			self:playcommandonchildren("ChildrenOn")
		end,
		ChildrenOnCommand = function(self)
			self:diffusealpha(0):sleep(0.5):linear(0.5):diffusealpha(1)
		end,
		LoadFont("Common Normal") ..
			{
				Text = getThemeName(),
				InitCommand = function(self)
					self:y(-24)
				end,
				OnCommand = function(self)
					self:sleep(1):linear(3):diffuse(color("#111111")):diffusealpha(0)
				end
			},
		LoadFont("Common Normal") ..
			{
				Text = "Created by " .. minanyms[math.random(#minanyms)],
				InitCommand = function(self)
					self:y(16):zoom(0.75):maxwidth(SCREEN_WIDTH)
				end,
				OnCommand = function(self)
					self:sleep(1):linear(3):diffuse(color("#111111")):diffusealpha(0)
				end
			}
	}
}

return t
