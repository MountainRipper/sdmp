if(rootPath == nil) then
    rootPath = '/Users/aimymusic/projects/sdp/script/'
end
package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
package.cpath = rootPath .. '?.so'..';'..rootPath .. 'lib/?.so'..';'..package.cpath ;

json = require('json')
graphModule = require('graph')

oo.class("Recorder", Graph)
Recorder.layer = '__layer_recorder'


function string.starts(String,Start)
    return string.sub(String,1,string.len(Start))==Start
 end

function Recorder:init()
	Recorder.super.init(self)

	self.filters={
		
		videoSource={
			module='mediaSourceMemory'
		},	
        audioSource={
			module='mediaSourceMemory'
		},
        videoEncoder={
            module='videoEncoderFFmpeg'
        },
        audioEncoder={
            module='audioEncoderFFmpeg'
        },
        mediaMuxer={
            module='mediaMuxerFFmpeg'
        }
	}
	self.info={
        uri = "",
        width = 0,
        height = 0,
        videoBitrate = 0;
        audioBitrate = 0;
	}
end


function Recorder:onConnectEvent()
	io.stderr:write( "======================Recorder:onConnectEvent======================\n")

    print_dump(self.info,"record info:")
    if(self.info.uri == '') then
        io.stderr:write( "Recorder:onConnectEvent failed: no uri set\n")
        return -1
    end
    
    local videoFormat = self:getFilterPinFormats(videoSource,false,1,1)
    print_dump(videoSource,"video format:")

    self.info.width = videoFormat.width
    self.info.height = videoFormat.height
    if(self.info.videoBitrate == 0) then        
        self.info.videoBitrate = self.info.width * self.info.height * 2;
    end
    if(self.info.audioBitrate == 0) then        
        self.info.audioBitrate = 128;
    end

    local adviceUri = self.info.uri
    if(string.starts(adviceUri,'rtmp')) then
        adviceUri = "dummy.flv"
    end

    self:setFilterProperty(mediaMuxer, 'uri', self.info.uri)
    self:setFilterProperty(videoEncoder, 'adviceUri', adviceUri)
    self:setFilterProperty(videoEncoder, 'bitrate', self.info.videoBitrate)
    
	self:connectAuto(videoSource,videoEncoder)
	self:connectAuto(audioSource,audioEncoder)
    self:connectAuto(videoEncoder,mediaMuxer)
    self:connectAuto(audioEncoder,mediaMuxer)
end

function Recorder:onStatusEvent( status )
    qsdpStatusEvent(qsdpContext,status)
end

function createGraph( )
    print_dump("createGraph:")
	--return player instance
	instance = oo.new(Recorder)
	return instance
end
