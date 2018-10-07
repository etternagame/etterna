local defaultConfig = {
	-- put here the added custom windows in the desired order(if you dont they wont rotate in the eval screen)
	customWindows = {"dpJ4", "osuManiaOD10"},
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
			marv = 16.0,
			perf = 34.0,
			great = 67.0,
			good = 97.0,
			boo = 121.0
		},
		judgeWeights = {
			marv = 3,
			perf = 3,
			great = 2,
			good = 1,
			boo = 0.5,
			miss = 0,
			holdHit = 0.5,
			holdMiss = -3,
			mineHit = 0
		}
	}
}

timingWindowConfig = create_setting("timingWindowConfig", "timingWindowConfig.lua", defaultConfig, -1)
timingWindowConfig:load()
