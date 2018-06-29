require 'pl'
class = require 'middleclass/middleclass'
Req = {}
function Req.using(envToAdd)
	local env = envToAdd
	setmetatable(env, {__index=getfenv(2)})
	setfenv(2, env)
end