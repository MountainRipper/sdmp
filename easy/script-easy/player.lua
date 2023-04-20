if(rootPath == nil) then
        rootPath = '/home/xuwei/work/MountainRipper/sdmp/script/'
end
package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
package.cpath = rootPath .. '?.so'..';'..rootPath .. 'lib/?.so'..';'..package.cpath ;

json = require('json')
graphModule = require('graph')

oo.class("Player", Graph)
Player.layer = '__layer_player'

playing = false

function mediaSoueceException(objectId,code)
	--qsdpIoExeptionEvent(qsdpContext,code)
end

function mediaSoueceCustomIOCheck(objectId,uri)
	if(mediaSource["protocol"] == "file") then		
		return uri
	end
	return ""
end

function Player:init()
	Player.super.init(self)

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
                        --uri = 'http://media.aimymusic.com/0042faffe7a8fad051a4107c96d806b3'
                        uri='/home/xuwei/work/resources/nonono.mp4'
                        --uri='/home/xuwei/work/resources/2tracks.mp4'
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
			modePullPush=false
		},
	}
	self.info={
		videoWidth=0,
		videoHeight=0,
		videoFps=0,
		audioChannels=0,
		audioSamplerate=0,
		audioBits=0,
		volume=1.0,
		channelMode = 0,
		duration=0,
                position=0,
		uri="",
		tracks=0,
		track = 0,
                timeout=30000,
                autoReplay = true
	}

	self.audioStreamHandlers = {
	}

	self.stepVolume = 0.02
        self.wantVolume = 1.0
	self.status = kStatusNone
	self.lastStatus = kStatusNone
	self.media = ''
	self.mediaLoaded = false 
        self.ignoreStatus = false
end

function Player:callNative(name,param)
	return luaCallNative(nativeContext,name, param)
end

function Player:onConnectEvent()
	io.stderr:write( "======================Player:onConnectEvent======================\n")
	
	for i = 1, #self.audioStreamHandlers do		
		self:removeFilter(self.audioStreamHandlers[i].audioDecoder.id)
		self:removeFilter(self.audioStreamHandlers[i].audioOutput.id)
	end

	self.tracks = 0
	self.info.track = 1
	self.info.channelMode = 0
	self.audioStreamHandlers = {}
	print_dump(self.audioStreamHandlers, '----------- dump source')
	for i = 1, #mediaSource.pinsOutput do
		if(mediaSource.pinsOutput[i].type == "audio") then
			self.tracks = self.tracks + 1

			local impVolume = self.wantVolume
			if (self.info.tracks ~= self.info.track) then
				impVolume = 0
			end

			local audioTrack = {}
			audioTrack.audioDecoder = {id='audioDecoder'..tostring(self.tracks), params={module='audioDecoderFFmpeg'}}
			audioTrack.audioOutput = {id='audioOutput'..tostring(self.tracks), 
								params={module='audioOutputParticipant',
								idEngine='defaultAudioPlaybackDevice',
								cacheDuration=1500,
								cacheHungerDuration=500,
								volume=impVolume}}

			local audioDecoder = self:createFilter(audioTrack.audioDecoder.id, audioTrack.audioDecoder.params)
			local audioOutput = self:createFilter(audioTrack.audioOutput.id, audioTrack.audioOutput.params)
            		self:connectAuto(mediaSource, audioDecoder)
			self:connectAuto(audioDecoder, audioOutput)
			self.audioStreamHandlers[self.tracks] = audioTrack
		end
	end

	

    	self:connectAuto(mediaSource,videoDecoder)
	self:connectAuto(videoDecoder,videoOutput)

	
	-- print_dump(graph, '-------- dump graph')
end

function Player:onConnectDoneEvent()
	io.stderr:write( "======================Player:onConnectDoneEvent====================== \n")	
	self:getInfo()	
end

local c = 0
function Player:onMasterClockEvent( )

end

function Player:onStatusEvent( status )
        local replayed = false
        if(status == kStatusEos and self.info.autoReplay) then
                print_dump("Player re-play")
                --replay seek and play may cause new status, ignore them
                self.ignoreStatus = true
		self:seek(0)
		self:play()
                self.ignoreStatus = false
                replayed = true
	end

        if(self.status == status or self.ignoreStatus) then
		return
	end 

        self.status = status

	if (self.status == kStatusPaused) then
                self:callNative("paused")
	elseif (self.status == kStatusStoped) then
                self:callNative("stoped")
	elseif (self.status == kStatusEos) then
                if(replayed) then
                    self:callNative("replaying")
                else
                    self:callNative("end-of-stream")
                end
	elseif (self.status == kStatusRunning) then
                self.info.volume = 0 -- fade in volume from 0 to wantVolume
		if (self.lastStatus == kStatusPaused) then
                        self:callNative("resume")
		elseif (self.lastStatus == kStatusReady) then
			if(self.mediaLoaded == false) then
				self.mediaLoaded = true
                                self:callNative("playing")
			else
                                -- mediaLoaded will not go to kStatusReady when graph already connected
                                -- reload media? stop -> disconnect -> connect -> ready
                                -- manual call connect, to re-connect filters ?
                                self:callNative("???")
			end
		else
                        --kStatusSeekReady
                        self:callNative("playing")
		end
	end
	self.lastStatus = status
end

function Player:onPositionEvent( position )

	local neadFade = false
	if(self.info.volume > self.wantVolume) then 
		self.info.volume = math.max(self.info.volume - self.stepVolume, self.wantVolume)
		neadFade = true
	end

	if(self.info.volume < self.wantVolume) then
		self.info.volume = math.min(self.info.volume + self.stepVolume, self.wantVolume)
		neadFade = true
	end

	if(neadFade == true) then
		for i = 1, #self.audioStreamHandlers do
			local audioTrack = self.audioStreamHandlers[i]
			local setVolume = 1.0
			if(self.info.track == i) then
				--fade in selected track
				setVolume = self.info.volume
			elseif(audioTrack.audioOutput.params.volume > 0) then
				-- switch track fade, fade out last track
				setVolume = self.wantVolume - self.info.volume				
			else 
				-- seeking fade in,other tracks already mute
				goto continue
			end
			audioTrack.audioOutput.params.volume = setVolume
			self:setFilterPropertyById(audioTrack.audioOutput.id, 'volume', setVolume)
			::continue::
		end 
	end

        self.info.position = position
        self:callNative("position",position)
end

function Player:getInfo( )	
	print_dump("Player:getInfo( )")
	local selectTrack = self.info.track
	self.info={
            videoWidth=0,
            videoHeight=0,
            videoFps=0,
            audioChannels=0,
            audioSamplerate=0,
            audioBits=0,
            volume=1.0,
            channelMode = 0,
            duration=0,
            position=0,
            uri="",
            tracks=0,
            track = 0,
            timeout=30000,
            autoReplay = true
	}
	self.info.track = selectTrack

	--mediaSourceExt.duration = 0
	
	local videoFormat = self:getFilterPinFormats(videoOutput,true,1,1)
	local audioFormat = nil
	if (#mediaSource.pinsOutput > 0) then 
		audioFormat = self:getFilterPinFormats(self.audioStreamHandlers[self.info.track].audioOutput.params,true,1,1)
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
	self:callNative("connect-done")
	return 0
end

function  Player:nativeSetMedia(media)
	print_dump(media, "Script-------------: Player:nativeSetMedia")
	self.mediaLoaded = false
	self.media = media
	graph:setFilterPropertyById('mediaSource','uri',self.media)
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

function Player:setAutoReplay(replay)
        self.info.autoReplay = replay
end

function Player:setTrack(track)
	print_dump(track, "nativeSetTrack : ")
	if(self.info.track ~= track) then 
		self.info.track = track;
                self.info.volume = 0 -- make it fade on position event <iner sound card>
                self:callNative("track-changed")
	end
end

function Player:setChannelMode(mode)
	if(self.info.channelMode ~= mode) then
		self.info.channelMode = mode
		local com = self.audioStreamHandlers[self.info.track]
		local mapping = {[0]={0,1},[1]={0,0},[2]={1,1}}
		self:setFilterPropertyById(com.audioOutput.id, 'channelMapping', mapping[self.info.channelMode])
	end
end

function Player:nativeSetTimeout(timeout)
	if(self.info.timeout ~= timeout) then
		self.info.timeout = timeout
		qsdpTimeoutEvent(qsdpContext, {value=self.info.timeout})
	end 
end

function Player:setVolume(volume)
	if(self.wantVolume ~= volume) then
		self.wantVolume = volume
	end
end

function nativeSetAutoReplay(replay)
        player:setAutoReplay(replay)
end
function nativeSetChannelMode(mode)
	player:setChannelMode(mode)
end
function nativeSetTrack(track)
	player:setTrack(track+1)
end
function nativeSetVolume(volume)
	player:setVolume(volume)
end
function nativeSetVideoEmitMode(modePullPush)
	player:setFilterPropertyById("videoOutput", 'modePullPush', modePullPush)
end

--a createGraph function to called by native C++,return 'graph' object
function createGraph( )
	--return player instance	
	player = oo.new(Player)
	return player
end

