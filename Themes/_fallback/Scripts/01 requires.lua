local current_dir=io.popen"cd":read'*l'
package.path = package.path .. ";"..current_dir.."\\lualibs\\?.lua;"..current_dir.."\\lualibs\\?\\init.lua"
require 'pl'
class = require 'middleclass/middleclass'
Req = {}
function Req.using(envToAdd)
	local env = envToAdd
	setmetatable(env, {__index=getfenv(2)})
	setfenv(2, env)
end