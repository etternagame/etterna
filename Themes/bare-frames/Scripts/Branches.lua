-- Branches determine the Screen loading logic for moving forward or backwards through screens.
-- Defining branches here is useful for writing logic in lua instead of metrics
-- Metrics points to functions found here or in the fallback branches.
-- but keep in mind the Branch table can be modified from any file (please keep it organized)

Branch.AfterSelectStyle = function()
    return "ScreenSelectMusic"
end