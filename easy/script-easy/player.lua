if(rootPath == nil) then
        rootPath = '/Users/aimymusic/projects/sdp/script/'
end
package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
package.cpath = rootPath .. '?.so'..';'..rootPath .. 'lib/?.so'..';'..package.cpath ;

json = require('json')
graphModule = require('graph')

oo.class("Player", Graph)
Player.layer = '__layer_player'

playing = false

function mediaSoueceException(objectId,code)
	qsdpIoExeptionEvent(qsdpContext,code)
end

function mediaSoueceCustomIOCheck(objectId,uri)
	if(mediaSource["protocol"] == "file") then		
		return uri
	end
	return ""
end

function Player:init()
	Player.super.init(self)

	print_dump(qsdpAudioGrabber, 'qsdpAudioGrabber')
	self.filters={
		-- two tracks http://aamresource.singworld.cn/BB/BBFB9043FA933EDDC7C737CCEBD65D16A3A83444F8288C010C7FF87D460E7623.mp4
		-- left/right channel  http://aamresource.singworld.cn/96/9631E5492FC009DB135A7EAFA30C7D3E1E3D072A4F4F54D8133D41E3B7776E35.mp4
		-- large frame cache   http://act-res.singworld.cn/rc-upload-1603162844650-44.mp4
		mediaSource={
			module='mediaSourceFFmpeg',
			customIO='f7b723d0-bd48-11eb-9155-4bbb70c16e00',
			customIOCheckHandler=mediaSoueceCustomIOCheck,
			exceptionHandler=mediaSoueceException,
                        --uri='',
                        -- uri='http://devmedia.aimymusic.com/02aebbde6571c5d9006a4bb7bfd62c1f'
                         uri = 'http://media.aimymusic.com/0042faffe7a8fad051a4107c96d806b3'
                        --uri='/home/xuwei/work/resources/alone.mp4'
		},	
		-- mediaCache={
		-- 	module='mediaCacheSaver',
		-- },
		videoDecoder={
			module='videoDecoderFFmpeg',
			-- module='videoDecoderRkmpp',
			hardwareApi=""
		},
		videoOutput={
			module='videoOutputProxy',
			modePullPush=true
		},
	}
	self.info={
		videoWidth=0,
		videoHeight=0,
		videoFps=0,
		audioChannels=0,
		audioSamplerate=0,
		audioBits=0,
		duration=0,
		uri="",
		tracks=0,
		timeout=30000,
		volume=1.0,
	}

	self.audioStreamHandlers = {
	}

	self.autoReplay = false 
	self.volume = 0 -- major iner sound card's volume
	self.channelMode = 0 -- major iner sound card's channel mode
	self.stepVolume = 0.01
	self.wantVolume = 1.0
	self.position = 0
	self.status = kStatusNone
	self.lastStatus = kStatusNone
	self.track = 0
	self.tracks = 0
	self.media = ''
	self.mediaLoaded = false 

end

function Player:onConnectEvent()
	io.stderr:write( "======================Player:onConnectEvent======================\n")
	
	qsdpOnConnect(qsdpContext)
	
	for i = 1, #self.audioStreamHandlers do		
		self:removeFilter(self.audioStreamHandlers[i].audioDecoder.id)
		self:removeFilter(self.audioStreamHandlers[i].audioOutput.id)
	end

	print_dump(mediaSource, '----------- dump source')

	self.audioStreamHandlers = {}
	self.tracks = 0

	local audioIndex =  1
	for i = 1, #mediaSource.pinsOutput do
		if(mediaSource.pinsOutput[i].type == "audio") then
			local impVolume = self.wantVolume
			if (audioIndex > 1) then
				impVolume = 0
			end
			local com = {}
			com.audioDecoder = {id='audioDecoder'..tostring(audioIndex), impl={module='audioDecoderFFmpeg'}}
			com.audioOutput = {id='audioOutput'..tostring(audioIndex), 
								impl={module='audioOutputParticipant',
								idEngine='defaultAudioPlaybackDevice',
								grabber=qsdpAudioGrabber,
								cacheDuration=2000,
								cacheHungerDuration=1500,
								volume=impVolume}}

			local audioDecoder = self:createFilter(com.audioDecoder.id, com.audioDecoder.impl)
			local audioOutput = self:createFilter(com.audioOutput.id, com.audioOutput.impl)

            self:connectAuto(mediaSource, audioDecoder)
			self:connectAuto(audioDecoder, audioOutput)
			self.audioStreamHandlers[audioIndex] = com
			audioIndex = audioIndex + 1
			self.tracks = self.tracks + 1
		end
	end

    self:connectAuto(mediaSource,videoDecoder)
	self:connectAuto(videoDecoder,videoOutput)

	self.track = 0
	qsdpTrackEvent(qsdpContext, {value=self.track})

	self.channelMode = 0
	qsdpChannelModeEvent(qsdpContext, {value=self.channelMode})

	-- print_dump(graph, '-------- dump graph')
end

function Graph:onConnectDoneEvent()
	io.stderr:write( "======================Player:onConnectDoneEvent====================== \n")
	self:getInfo()
end

local c = 0
function Player:onMasterClockEvent( )

end

function Player:onStatusEvent( status )
	if(status == kStatusEos and self.autoReplay) then	
		print_dump("Player re-play")	
		self:seek(0)
		self:play()
	end

	if(self.status == status) then
		return
	end 

	self.status = status
	qsdpStatusEvent(qsdpContext, {value=self.status})

	if (self.status == kStatusPaused) then
		qsdpPlayPaused(qsdpContext)
	elseif (self.status == kStatusStoped) then
		qsdpPlayStoped(qsdpContext)
	elseif (self.status == kStatusEos) then
		qsdpPlayGetEnd(qsdpContext)
	elseif (self.status == kStatusRunning) then
		self.volume = 0 -- fade volume from 0 to wantVolume
		if (self.lastStatus == kStatusPaused) then
			qsdpPlayResumed(qsdpContext)
		elseif (self.lastStatus == kStatusReady) then
			if(self.mediaLoaded == false) then
				self.mediaLoaded = true
				qsdpPlayStart(qsdpContext)
			else
				qsdpPlayRestart(qsdpContext)
			end
		else
			qsdpPlayStart(qsdpContext)
		end
	-- elseif (self.status == kStatusReady) then 
	-- 	if(self.lastStatus == kStatusInit) then
	-- 		qsdpMediaLoaded(qsdpContext)
	-- 	end
	end
	self.lastStatus = status

end

function Player:onPositionEvent( position )

	--print_dump(self.volume, 'onPositionEvent')
	-- linear fade volume
	-- print_dump(mediaSource)
	-- print_dump(position, 'Player:onPositionEvent')
	local neadFade = false
	if(self.volume > self.wantVolume) then 
		self.volume = math.max(self.volume - self.stepVolume, self.wantVolume)
		neadFade = true
	end

	if(self.volume < self.wantVolume) then
		self.volume = math.min(self.volume + self.stepVolume, self.wantVolume)
		neadFade = true
	end

	if(neadFade == true) then
		for i = 1, #self.audioStreamHandlers do
			-- print_dump(i, '...................')
			local com = self.audioStreamHandlers[i]
			local setVolume = 1.0
			if((self.track + 1) == i) then
				setVolume = self.volume
			else
				setVolume = self.wantVolume - self.volume
			end
			self:setFilterPropertyById(com.audioOutput.id, 'volume', setVolume)
		end 
	end

	self.position = position
	qsdpPositionEvent(qsdpContext, {value=self.position})
end

function Player:getInfo( )	
	print_dump("Player:getInfo( )")
	self.info={
		videoWidth=0,
		videoHeight=0,
		videoFps=0,
		audioChannels=0,
		audioSamplerate=0,
		audioBits=0,
		duration=0,
		uri="",		
		tracks=0,
		timeout=5000,
		volume=1.0
	}
	--mediaSourceExt.duration = 0
	
	local videoFormat = self:getFilterPinFormats(videoOutput,true,1,1)
	local audioFormat = nil
	if (#mediaSource.pinsOutput > 0) then 
		audioFormat = self:getFilterPinFormats(self.audioStreamHandlers[1].audioOutput.impl,true,1,1)
	end
	self.info.duration = mediaSource.duration	
	self.info.tracks = self.tracks
	self.info.timeout = mediaSource.timeout
	self.info.volume = self.wantVolume

	if(videoFormat ~= nil) then
		self.info.videoWidth = videoFormat.width
		self.info.videoHeight = videoFormat.height
		self.info.videoFps = videoFormat.fps
	end
	if(audioFormat ~= nil) then
		self.info.audioSamplerate = audioFormat.samplerate
		self.info.audioChannels = audioFormat.channels
		self.info.audioBits = audioFormat.format_bpp
	end

	self.info.uri = mediaSource.uri
	self.media = mediaSource.uri

	print_dump(self.info,"Player Formats:")
	qsdpGetInfo(qsdpContext,self.info)
	return 0
end

function  Player:nativeSetMedia(media)
	print_dump(media, "Script-------------: Player:nativeSetMedia")
	self.mediaLoaded = false
	self.media = media
	graph:setFilterPropertyById('mediaSource','uri',self.media)
end

function  Player:nativeSetPosition(position)
	self:seek(position)
end

function  Player:nativeSetCommand(command)
	if(command ~= "") then
		if(command == "play") then
			self:play()
		elseif(command == "pause") then
			self:pause()
		elseif(command == "stop") then
			self:stop()
		end
	end
end

function Player:nativeSetAutoReplay(replay)
	self.autoReplay = replay
	qsdpAutoReplayEvent(qsdpContext, {value=self.autoReplay})
end

function Player:nativeSetTrack(track)
	print_dump(track, "nativeSetTrack : ")
	if(self.track ~= track) then 
		self.track = track;
		self.volume = 0 -- make it fade on position event <iner sound card>
		qsdpTrackEvent(qsdpContext, {value=self.track})
	end
end

function Player:nativeSetChannelMode(mode)
	if(self.channelMode ~= mode) then
		self.channelMode = mode
		local com = self.audioStreamHandlers[self.track + 1]
		local mapping = {[0]={},[1]={0,0},[2]={1,1}}
		self:setFilterPropertyById(com.audioOutput.id, 'channelMapping', mapping[self.channelMode])
		qsdpChannelModeEvent(qsdpContext, {value=self.channelMode})
	end
end

function Player:nativeSetTimeout(timeout)
	if(self.info.timeout ~= timeout) then
		self.info.timeout = timeout
		qsdpTimeoutEvent(qsdpContext, {value=self.info.timeout})
	end 
end

function Player:nativeSetVolume(volume)
	print_dump(volume, "Player:nativeSetVolume")
	if(self.wantVolume ~= volume) then
		self.wantVolume = volume
		qsdpVolumeEvent(qsdpContext, {value=self.wantVolume})
	end
end

function Player:nativeSetVideoEmitMode(modePullPush)
	self:setFilterPropertyById("videoOutput", 'modePullPush', modePullPush)
end

--a createGraph function to called by native C++,return 'graph' object
function createGraph( )
	--return player instance
	player = oo.new(Player)
	return player
end
