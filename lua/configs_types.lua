-- dcplaya - lua well-known config types
--
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/14
--
--
-- $Id: configs_types.lua,v 1.1 2002-10-15 06:02:47 benjihan Exp $
--

-- Unload library
configs_types_loaded = nil

-- Load required libraries
if not dolib("basic") then return end
if not dolib("color") then return end

-- Used clipping functions
--

-- Clip integer
function configs_types_integer_clip(v,min,max)
	if type(v) ~= "number" then return end
	if type(min) == "number" then min = floor(min) end
	if type(max) == "number" then max = floor(max) end
	return clip_value(floor(v),min,max)
end

-- Clip boolean
function configs_types_boolean_clip(v,min,max)
	if type(v) ~= "number" or v==0 then return 0 else return 1 end
end

-- Clip string
function configs_types_string_clip(v,min,max)
	if type(v) ~= "string" then return end
	local l=strlen(v)
	if type(min) == "number" and l < min then return end
	if type(max) == "number" and l > max then return strsub(v,1,max)
	else return v end
end

-- Clip percent
function configs_types_percent_clip(v)
	if type(v) ~= "number" then return end
	return clip_value(v,0,1)
end

-- Table of well-known configs type field
-- 
configs_types = {
	["integer"] 	=	{	clip = configs_types_integer_clip
						},
	["boolean"] 	=	{	clip = configs_types_boolean_clip
						},
	["color"] 		=	{	default = color_new(),
							clip = color_clip
						},
	["string"] 		=	{	clip = configs_types_string_clip
						},
	["percent"] 	=	{	clip = configs_types_percent_clip
						},
}

-- Retrieve a well known config field of given type
--
function configs_types_get(name)
	if configs_types[name] then
		local result = dup(configs_types[name])
		result.type = name
		return result
	end
end

configs_types_loaded = 1
return configs_types_loaded
