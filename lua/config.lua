-- dcplaya - lua config object
--
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/14
--
-- $Id: config.lua,v 1.1 2002-10-14 23:31:17 benjihan Exp $
--
----------------------------------------------------------------------
-- "configs" object description
--
--  name:    Describes what are these configs for. e.g "fftvlr"
--  fields:  Table of valid "field" for each "config" object
--           field := { type=STRING, [min=??, max,??] }
--           e.g { border = {type="INTEGER",min=0},
--                 ambient = {type="COLOR", min=-1, max=1} }
--  default: name of the default "config"
--  config:  Table of "config" object
--
--
-- "config" object description
--
--  name:    The name of this config. e.g. "yellow no border"
--  value:   Table of value indexed by a string derivated from configs.field.
--           e.g { ["_border"]=1, ["_ambient"]={1,1,1,1} }
--

-- Unload library
config_loaded = nil

-- Load required libraries
--
if not dolib("color") then return end

-- Create a new tag for "configs" and "config" (once).
--
if not configs_tag then configs_tag = newtag() end
if not config_tag  then config_tag  = newtag() end

-- Check for a valid "configs" object
--
function configs_check(cnfs)
	if tag(cnfs) == configs_tag then
		return 1
	end
	print("configs_check: not a valid configs object")
end

-- Check for a valid "config" object within a "configs" object
--
function config_check(cnfs, cnf)
	if not configs_check(cnfs) then return end
	if tag(cnf) ~= config_tag then
		print("config_check: not a valid config object")
	end
-- $$$ TODO
	return 1
end

-- Save a "configs" object into a lua script
--
function configs_save(cnfs,fname)
	if not configs_check(cnfs) then return end

	file = openfile(fname, "w")
	if not file then return end

	write(file,format(
[[
-- %q configuration file
--
-- %d configuration items.

return ]],cnfs.name,getn(cnfs.fields)))

	write(file,type_dump(cnfs))
	write(file,format(
[[

-- end of %q file

]], cnfs.name))

	closefile(file)
	print(format("config %q saved in %q",cnfs.name, fname))
	return 1
end

function configs_load(fname)
	local cnfs,c,i

	cnfs = dofile(fname)
	if type(cnfs) == "table" then
		settag(cnfs, configs_tag)
		if not cnfs.fields then cnfs.fields={} end
		if not cnfs.config then cnfs.config={} end
		for i,c in cnfs.config do
			settag(c, config_tag)
		end
		if not cnfs.default or not cnfs.config[cnfs.default] then
			local dc = rawget(cnfs.config,1)
			if dc then cnfs.default = dc.name end
		end
	end
	return cnfs
end

-- Create a "configs" object
--
function configs_create(name, fields)
	if type(name) ~= "string" or type(fields) ~= "table" then
		print("configs_create : bad arguments type")
		return
	end
	local cnfs = { name=name, fields={}, configs={} }
	local i,f,j,v
	for i,f in fields do
		if type(i) ~= "string" then
			print(format("configs_create %q : unnamed field %q ",
							name, tostring(i)))
			return
		end
		if type (f) ~= "table" then
			print(format("configs_create %q : field %q not a table",name, i))
			return
		end
		if not f.type then
			print(format("configs_create %q : field %q as no %q",
					name, i, "type"))
			return
		end
		cnfs.fields[i] = {}
		for j,v in f do
			cnfs.fields[i][j] = v
		end
	end
	settag(cnfs, configs_tag)
	return cnfs
end

function configs_add(cnfs, config)
	if not configs_check(cnfs) then return end
	if not config_check(cnfs, config) then return end
end

function configs_del(cnfs, config)
	if not configs_check(cnfs) then return end
	if not config_check(cnfs, config) then return end
end

function configs_index(cnfs, i)
	if not configs_check(cnfs) then return end
	if not tag(i) ~= config_tag then return end
	return cnfs[i.name]
end

conf=configs_create("test-config",
	{
		titi={type="titi", min=4},
		toto={type="zob", value="yoyo"}
	})
print(type_dump(conf))

configs_save(conf,"/ram/c")

config_loaded = 1
return config_loaded

