local easingFunctions = require('easing');

defualtEasings = {
  easingFunctions.linear ,
  easingFunctions.inQuad ,
  easingFunctions.outQuad ,
  easingFunctions.inOutQuad ,
  easingFunctions.outInQuad ,
  easingFunctions.inCubic  ,
  easingFunctions.outCubic ,
  easingFunctions.inOutCubic ,
  easingFunctions.outInCubic ,
  easingFunctions.inQuart ,
  easingFunctions.outQuart ,
  easingFunctions.inOutQuart ,
  easingFunctions.outInQuart   ,
  easingFunctions.inSine       ,
  easingFunctions.outSine      ,
  easingFunctions.inOutSine    ,
  easingFunctions.outInSine    ,
  easingFunctions.inCirc       ,
  easingFunctions.outCirc      ,
  easingFunctions.inBounce     ,
  easingFunctions.outBounce    ,
  easingFunctions.outInBack,
  easingFunctions.outInElastic
}

defaultAreas = {	
	----------------
	----scale to full
	---------------- 
	{ from = {x=0.5, y=0.5, s=0.8}, to = {x=0.5, y=0.5, s=1.0} },
	{ from = {x=0.4, y=0.5, s=0.8}, to = {x=0.6, y=0.5, s=0.8} },
	{ from = {x=0.5, y=0.4, s=0.7}, to = {x=0.5, y=0.6, s=0.7} },
	{ from = {x=0.5, y=0.4, s=0.6}, to = {x=0.5, y=0.5, s=0.8} },
	{ from = {x=0.4, y=0.5, s=0.8}, to = {x=0.5, y=0.5, s=0.6} },

	}

function get_easing(value_from,value_to,progress,easingFunc)
	local diff = value_to - value_from;
	local progress_easing = easingFunc(progress,0,1,1);
	local result = value_from + progress_easing * diff;
	return result;
end

--{x,y,s} as center x,center y, scale
function kenburnsCalc(from,to,progress,easingFunc)
	local result = {}
	result.x = get_easing(from.x, to.x, progress, easingFunc)
	result.y = get_easing(from.y, to.y, progress, easingFunc)
	result.s = get_easing(from.s, to.s, progress, easingFunc)
	return result
end

function check_value(item,name)
	half = item.s / 2;
	left = item.x - half;
	right = item.x + half;
	bad_x = false
	if(left < 0 or right > 1) then
		bad_x = true;
	end

	left = item.y - half;
	right = item.y + half;
	bad_y = false
	if(left < 0 or right > 1) then
		bad_y = true;
	end

	if(bad_x or bad_y) then
		print(name," bad item",item.x,item.y,item.s)
	end

end

function make_revers_animation( )
	revers_list = {}

	for key,value in ipairs(defaultAreas) do
		check_value(value.from,'from')
		check_value(value.to,'to')
	end

	for key,value in ipairs(defaultAreas) do
		local revers_item = {from=value.to, to=value.from};
		table.insert(revers_list,revers_item);
	end

	for key,value in ipairs(revers_list) do
		table.insert(defaultAreas,value)
	end

end



make_revers_animation();

print("all areas count:",#defaultAreas);
print("easing count:",#defualtEasings);

function randomArea( )
	local area = defaultAreas[math.random(1,#defaultAreas)];
	return area;
end

function randomEasing( )
	local easing = defualtEasings[math.random(1,#defualtEasings)];
	return easing;
end



kenburnDefines = {
	calc = kenburnsCalc,
	randomArea = randomArea,
	randomEasing = randomEasing,
	areaDefines = defaultAreas,
	easingDefines = defualtEasings
};

return kenburnDefines
