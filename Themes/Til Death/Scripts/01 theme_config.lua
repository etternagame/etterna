local defaultConfig = {
	global = {
		TipType = 1, -- 1 = Hide,2=tips 3= random quotes phrases,
		RateSort = true,
		HelpMenu = false,
		MeasureLines = false,
		ProgressBar = 1, -- 0 = bottom, 1 = top
		ShowVisualizer = true,
		InstantSearch = true, -- true = search per press, false = search on enter button
		IgnoreTabInput = 1, -- 1 = dont ignore, 2 = ignore only in search, 3 = always
		JudgmentTween = false,
		ComboTween = false,
		CenteredCombo = false,
		FadeNoteFieldInSyncMachine = true,
		ShowPlayerOptionsHint = true,
		ShowBanners = true, -- false to turn off banners everywhere
	},
	NPSDisplay = {
		MaxWindow = 2,
		MinWindow = 1 -- unused.
	},
}

themeConfig = create_setting("themeConfig", "themeConfig.lua", defaultConfig, -1)
themeConfig:load()

function JudgementTweensEnabled()
	return themeConfig:get_data().global.JudgmentTween
end
function ComboTweensEnabled()
	return themeConfig:get_data().global.ComboTween
end
function CenteredComboEnabled()
	return themeConfig:get_data().global.CenteredCombo
end
function BannersEnabled()
	return themeConfig:get_data().global.ShowBanners
end