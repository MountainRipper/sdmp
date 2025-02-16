-- 将当前脚本路径加入搜索路径
package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
local info = debug.getinfo(1, "S")
local path = string.match(string.sub(info.source, 2), "^.*/")
package.path = path .. '?.lua;'..path .. 'lib/?.lua;'..package.path ;
io.stderr:write(">>>>>>> package.path: "..package.path)

-- json = require('json')
graphModule = require('graph')


oo.class("Publisher", Graph)
Publisher.layer = '__layer_publisher'

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



function Publisher:init()
        Publisher.super.init(self)

        self.filters={
                mediaSource={
                        module='mediaSourceFFmpeg',
                        exceptionHandler=mediaSoueceException,
                        uri='/home/xuwei/Downloads/test.aac'
                },
                audioDecoder={
                    module='audioDecoderFFmpeg',
                },
                audioEncoder={
                    module='audioEncoderFFmpeg',
                    encoderName='libmp3lame',
                    bitrate=192000
                },
                dataGrabber={
                    module='dataGrabber',
                },
                audioOutput={
                    module='audioOutputParticipant',
                    idEngine='defaultAudioPlaybackDevice',
                    cacheDuration=1500,
                    cacheHungerDuration=500
                },
                nullOutput={
                        module='nullOutput'
                },
                muxer = {
                        module='mediaMuxerFFmpeg',
                        uri='test.mp3'

                }
        }

        self.stepVolume = 0.02
        self.wantVolume = 1.0
        self.status = kStatusNone
        self.lastStatus = kStatusNone
        self.media = ''
        self.mediaLoaded = false
        self.ignoreStatus = false
end

function Publisher:callNative(name,param)
        return luaCallNative(nativeContext,name, param)
end

function Publisher:onConnectEvent()
    self:connectAuto(mediaSource,audioDecoder)

--     self:connectAuto(audioDecoder,audioOutput)

    self:connectAuto(audioDecoder,audioEncoder)
    self:connectAuto(audioEncoder,dataGrabber)
    self:connectAuto(dataGrabber,nullOutput)
end


--a createGraph function to called by native C++,return 'graph' object
function createGraph( )
        --return player instance
        publisher = oo.new(Publisher)
        return publisher
end
