local t = Def.ActorFrame {Name = "SearchFile"}

local ratios = {
    Width = 782 / 1920,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything
    SliderColumnLeftGap = 196 / 1920, -- from left edge of text to left edge of sliders
    RightColumnLeftGap = 410 / 1920, -- from left edge of frame to left edge of text
    -- using the section height to give equidistant spacing between items with "less" work
    UpperSectionHeight = 440 / 1080, -- from bottom of upperlip to top of upper divider
    LowerSectionHeight = 485 / 1080, -- from bottom of upper divider to bottom of frame
}

local actuals = {

}

return t