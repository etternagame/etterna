local newdecoder = require 'lunajson.decoder'
local newencoder = require 'lunajson.encoder'
-- If you need multiple contexts of decoder and/or encoder,
-- you can require lunajson.decoder and/or lunajson.encoder directly.
return {
	decode = newdecoder(),
	encode = newencoder(),
}
