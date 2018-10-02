local defaultConfig = {
	-- put here the added custom windows in the desired order(if you dont they wont rotate in the eval screen)
	customWindows = {"dpJ4", "osuManiaOD10", "osuManiaOD9", "osuManiaOD8"},
	dpJ4 = {
		name = "DP Judge 4",
		-- if you dont set judgeNames, defaults will be used, see next example for names
		judgeWindows = {
			marv = 22.5,
			perf = 45.0,
			great = 90.0,
			good = 135.0,
			boo = 180.0
		},
		judgeWeights = {
			marv = 2,
			perf = 2,
			great = 1,
			good = 0,
			boo = -4,
			miss = -8,
			holdHit = 6,
			holdMiss = -6,
			mineHit = -8
		}
	},
	-- this is a mere example, dont take it too seriously
	osuManiaOD10 = {
		name = "osu!mania OD10",
		judgeNames = {
			marv = "300g",
			perf = "300",
			great = "200",
			good = "100",
			boo = "50",
			miss = "Miss"
		},
		judgeWindows = {
			marv = 16.5,
			perf = 34.5,
			great = 67.5,
			good = 97.5,
			boo = 121.5
		},
		judgeWeights = {
			marv = 3,
			perf = 3,
			great = 2,
			good = 1,
			boo = 0.5,
			miss = 0,
			holdHit = 3,
			holdMiss = 0,
			mineHit = 0
		}
	}, 
	osuManiaOD9 = {
		name = "osu!mania OD9",
		judgeNames = {
			marv = "300g",
			perf = "300",
			great = "200",
			good = "100",
			boo = "50",
			miss = "Miss"
		},
		judgeWindows = {
			marv = 16.5,
			perf = 37.5,
			great = 70.5,
			good = 100.5,
			boo = 124.5
		},
		judgeWeights = {
			marv = 3,
			perf = 3,
			great = 2,
			good = 1,
			boo = 0.5,
			miss = 0,
			holdHit = 3,
			holdMiss = 0,
			mineHit = 0
		}
	},
	osuManiaOD8 = {
		name = "osu!mania OD8",
		judgeNames = {
			marv = "300g",
			perf = "300",
			great = "200",
			good = "100",
			boo = "50",
			miss = "Miss"
		},
		judgeWindows = {
			marv = 16.5,
			perf = 40.5,
			great = 73.5,
			good = 103.5,
			boo = 127.5
		},
		judgeWeights = {
			marv = 3,
			perf = 3,
			great = 2,
			good = 1,
			boo = 0.5,
			miss = 0,
			holdHit = 3,
			holdMiss = 0,
			mineHit = 0
		}
	}
}

timingWindowConfig = create_setting("timingWindowConfig", "timingWindowConfig.lua", defaultConfig, -1)
timingWindowConfig:load()
