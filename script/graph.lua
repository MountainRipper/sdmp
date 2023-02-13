if(rootPath == nil) then
        rootPath = '/Users/aimymusic/projects/sdp/script/'
end
package.path = rootPath .. '?.lua'..';'..rootPath .. 'lib/?.lua'..';'..package.path ;
package.cpath = rootPath .. '?.so'..';'..rootPath .. 'lib/?.so'..';'..package.cpath ;

--module require
inspect = require('inspect')
oo = require('bhou.oo.base')


kStatusNone   = 0
kStatusInit   = 1
kStatusReady  = 2
kStatusRunning= 3
kStatusPaused = 4
kStatusStoped = 5
kStatusEos    = 6

AVMEDIA_TYPE_UNKNOWN = -1
AVMEDIA_TYPE_VIDEO = 0
AVMEDIA_TYPE_AUDIO = 1
AVMEDIA_TYPE_DATA = 2
AVMEDIA_TYPE_SUBTITLE = 3
AVMEDIA_TYPE_ATTACHMENT = 4

kGraphCommandDisconnect    = "cmdDisconnect"
kGraphCommandPlay          = "cmdPlay"
kGraphCommandPause         = "cmdPause"
kGraphCommandStop          = "cmdStop"
kGraphCommandSeek          = "cmdSeek"
kGraphCommandClose         = "cmdClose"
kGraphOperatorConnect      = "doConnect"
kGraphOperatorCreateFilter = "doCreateFilter"
kGraphOperatorRemoveFilter = "doRemoveFilter"

--Object Oriented declear
local Graph = oo.class("Graph", BaseClase)
Graph.layer='__layer_graph_base'

function onScriptError (message)
	io.stderr:write( "LUA script error: " .. message.."\n")
end

function Graph:init()
	self.filters={} --initialnize filters
	self.loopsInterval=5--master clock loop interval in millieseconds

	print_dump(hostOS..'-'..hostARCH,"Create Graph On System:")
	print_dump(hostFeature,"Host Features:")
end

--helper functions 
function Graph:invoke( obj,functionName,... )
	if(obj == nil) then
		return
	end

	if(obj[functionName] == nil) then
		return
	end

	return obj[functionName](obj.context,...)
end

function Graph:setter(obj,property,value )	
	if(obj == nil or obj.context == nil or property == nil or value == nil) then
		return
	end

	obj.set(obj.context,property,value)
end


function Graph:getFilter( id )
	return self.filters[id]
end

function Graph:setFilterProperty(filter,property,value )	
	if(filter ~= nil) then
		filter.setProperty(filter.context,property,value)
	end
end

function Graph:setFilterPropertyById(id,property,value )	
	local filter = self:getFilter(id)
	self:setFilterProperty(filter,property,value)
end

function Graph:getFilterPin(filter,inOut,pinIndex)
	local pins = nil
	if(inOut) then
		pins = filter.pinsInput
	else
		pins = filter.pinsOutput
	end

	if(pins == nil) then
		return nil
	end

	if(pinIndex > #pins) then
		return nil
	end

	return pins[pinIndex]
end
--return formats arr if formatIndex<=0
function Graph:getFilterPinFormats(filter,inOut,pinIndex,formatIndex)
	local pin = self:getFilterPin(filter,inOut,pinIndex)
	
	if(pin == nil)then
		return nil
	end

	if(pin.formats == nil)then
		return nil
	end

	if(formatIndex <= 0)then
		return pin.formats
	end
	
	if(formatIndex > #pin.formats) then
		return nil
	end

	return pin.formats[formatIndex]
end

function Graph:setFilterPinActive(filter,inOut,pinIndex,active)
	local pin = self:getFilterPin(filter,inOut,pinIndex)
	
	if(pin == nil)then
		return nil
	end

	print_dump(pin)
	self:invoke(pin,"setActive",active)
end
--event handles
function Graph:onCreated( )
	self:onCreatedEvent()
end
function Graph:onMasterClock( )
	self:onMasterClockEvent()
end
function Graph:onConnect( )
	self:onConnectEvent()
end

function Graph:onConnectDone( )
	self:onConnectDoneEvent()
end

--graph  = 'graph'
--filter = 'filter.id'
function Graph:onError( objectId, code )
	self:onErrorEvent(objectId, code)
end

function Graph:onStatus( status )
	self:onStatusEvent(status)
end
function Graph:onPosition( positon )
	self:onPositionEvent(positon)
end

--event to overwrite
function Graph:onCreatedEvent( )
end
function Graph:onConnectEvent()
end
function Graph:onConnectDoneEvent()
end
function Graph:onMasterClockEvent()
end
function Graph:onErrorEvent( objectId,code )
end
function Graph:onStatusEvent( status )
end
function Graph:onPositionEvent( position )
end
--graph common base functions, call to c++
function Graph:play( )
	self:invoke(self,kGraphCommandPlay)
end

function Graph:pause( )
	self:invoke(self,kGraphCommandPause)
end

function Graph:seek( ms )
	self:invoke(self,kGraphCommandSeek,ms)
end

function Graph:stop()
	self:invoke(self,kGraphCommandStop)
end

function Graph:createFilter(id,filter)
	filter.id = id	
	if(self.filters[id] ~= nil) then
		return self.filters[id]
	end
	-- add to self:filters first to repair local creating
	self.filters[id] = filter
	local successed = self:invoke(self,kGraphOperatorCreateFilter,id,filter)	
	if(successed < 0) then
		-- create failed remove from self:filters
		self.filters[id] = nil
		return nil
	end
	return self.filters[id]
end

function Graph:removeFilter(id)
	if(self.filters[id] == nil) then
		-- filter not exist
		return 1
	end
	return self:invoke(self,kGraphOperatorRemoveFilter,id)	
end


function Graph:connect( filterSender,filterReceiver, senderPin, receiverPin)
	self:invoke(self,kGraphOperatorConnect,filterSender.context,filterReceiver.context,senderPin,receiverPin)
end

function Graph:connectAuto( filterSender,filterReceiver)
	self:connect(filterSender,filterReceiver,-1,-1)
end

function Graph:disconnectAll( )
	self:invoke(self,kGraphCommandDisconnect)
end

