local defaultConfig = {
	global = {
		TipType = 1, -- 1 = tips, 2 = quotes ...
		ScoreBoardNag = false,
		ProgressBar = 1, -- 0 = bottom, 1 = top
		ShowVisualizer = true,
		ShowBackgrounds = true,
		WheelPosition = true, -- true = left, false = right
		WheelBanners = true, -- true = on, false = off
		InstantSearch = true, -- true = search per press, false = search on enter button
	},
	eval = {
		CurrentTimeEnabled = true,
		JudgmentBarEnabled = true,
		ScoreBoardEnabled = true,
		ScoreBoardMaxEntry = 10,
		SongBGType = 1 -- 1 = song bg, 2 = grade+common, 3 = grade only
	},
	color = {
		main = "#00AEEF"
	}
}

themeConfig = create_setting("themeConfig", "themeConfig.lua", defaultConfig, -1)
themeConfig:load()

function getWheelPosition()
	-- true means left, false means right
	return themeConfig:get_data().global.WheelPosition
end
function useWheelBanners()
	return themeConfig:get_data().global.WheelBanners
end