-- The real point to this file is to capture input
-- And also to make the preview notefield look a little cleaner

-- I'm doing this in a really stupid way, trust me theres a reason why
-- (Hint: its because of input)
local p = Def.ActorFrame {}
p[#p+1] = LoadActor("_chartpreviewenv")
return p