if(rootPath == nil) then
    rootPath = '/Users/aimymusic/projects/sdp/script/'
end
package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
package.cpath = rootPath .. '?.so'..';'..rootPath .. 'lib/?.so'..';'..package.cpath ;

graphModule = require('graph')

oo.class("VideoDecoder", Graph)
VideoDecoder.layer = '__layer_recorder'


function string.starts(String,Start)
    return string.sub(String,1,string.len(Start))==Start
 end

function VideoDecoder:init()
	VideoDecoder.super.init(self)

	self.filters={
		
		videoSource={
			module='mediaSourceMemory'
		},	
        videoDecoder={
            module='videoDecoderFFmpeg',
            hardwareApi=""
        },
        videoOutput={
			module='videoOutputProxy',
			modePullPush=false,
			withoutSync=true
		},
	}
	self.info={
       
	}
end


function VideoDecoder:onConnectEvent()
	io.stderr:write( "======================VideoDecoder:onConnectEvent======================\n")
	self:connectAuto(videoSource,videoDecoder)
	self:connectAuto(videoDecoder,videoOutput)
end

function VideoDecoder:onStatusEvent( status )
    qsdpStatusEvent(qsdpContext,status)
end

function createGraph( )
    print_dump("createGraph:")
	--return decoder instance
	instance = oo.new(VideoDecoder)
	return instance
end
