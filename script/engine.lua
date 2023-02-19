package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
package.cpath = rootPath .. '?.so'..';'..rootPath .. 'lib/?.so'..';'..package.cpath ;

local inspect = require('inspect')

function setter(obj,property,value )	
	obj.set(obj.context,property,value)
end

audioOutputs={
	defaultAudioPlaybackDevice={
		module='miniaudioOutput',
		selector='',
		backend='alsa',
		samplerate=48000,
		channels=2,
		framesize=480,
		bits=32,
	}
}	

function test()
print_dump(defaultAudioPlaybackDevice,"defaultAudioPlaybackDevice")
end
