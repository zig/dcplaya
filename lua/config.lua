-- dcplaya - lua config object
--
-- author : benjamin gerard <ben@sashipa.com>
-- date   : 2002/10/14
--
-- $Id: config.lua,v 1.2 2002-10-15 06:02:47 benjihan Exp $
--
-- -------------------------------------------------------------------
-- "configs" object description
--
--  name:    Describes what are these configs for. e.g "fftvlr"
--  field:   Table of valid "field" for each "config" object
--            field := { type=TYPE,
--                      [default=X,] X is a valid default value
--                      [notnil=1,]  This field is required or "defaulted"
--                      [min=X,]     Minimum value (used by clip)
--                      [max=X,]     Maximum value (used by clip)
--                      [clip(v,min,max),] a clipping function.
--                      [name=X,...] Any other user field
--                     }
--            TYPE := [
--                      "integer"    Any integer number
--                      "boolean"    A number 0 or 1
--                      "color"      A ARGB color {a,r,g,b}
--                      "string"     min,max and clip work on string length
--                      "percent"    a number in the range [0..1]
--                    ]
--            e.g { border = {type="INTEGER",min=0},
--                  ambient = {type="COLOR", min=-1, max=1} }
--  default: name of the default "config"
--  config:  Table of "config" object
--
-- "config" object description
--           e.g { ["border"]=1, ["ambient"]={1,1,1,1} }
--

-- Unload library
config_loaded = nil

-- Load required libraries
--

-- $$$ FORCE LOAD FOR DEBUG !!!
if not dolib("configs_types",1) then return end

-- Create a new tag for "configs" and "config" (once).
--
if not configs_tag then configs_tag = newtag() end
-- $$$ May be this is not very useful because config may be differente
--     depending on "configs". Each "configs" may have a "config_tag" of its
--     own. Anyway, it is used to check the "name" and "field" components.
if not config_tag  then config_tag  = newtag() end


-- Check for a valid "configs" object
--
function configs_check(cnfs)
	if tag(cnfs) == configs_tag then
		return 1
	end
	print("configs : not a valid configs object")
end

-- Check for a valid "config" object within a "configs" object
--
function config_check(cnfs, cnf)
	if not configs_check(cnfs) then return end
	if tag(cnf) ~= config_tag then
		print(format("configs %q : %q not a valid config object",
				cnfs.name, tostring(cnf)))
		return
	end
	return configs_compatible(cnfs,cnf)
end

function configs_compatible(cnfs, cnf)
	if not configs_check(cnfs) then return end
	if type(cnf) ~= "table" then
		print(format("configs %q : %q not a table object",
				cnfs.name, tostring(cnf)))
		return
	end

--	print(format("configs %q : checking compatibility", cnfs.name))

	-- Checks all fields 
	local i,f,result
	result = {}
	for i,f in cnfs.field do
--		print("checking "..i)
		result[i] = cnf[i]
		if not result[i] then
--			print("not defined")
			result[i] = f.default
		end
		if type(f.clip) == "function" then
--			print("clipping "..tostring(result[i]).." via "..getinfo(f.clip).name)
			result[i] = f.clip(result[i],f.min,f.max)
		end
		if f.notnil and not result[i] then
			print(format("configs %q : %q missing in action", cnfs.name,i))
			return
		end
--		print("-->"..tostring(cnf[i]))
	end
	settag(result, config_tag)
	return result
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

return ]],cnfs.name,getn(cnfs.field)))

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
	if type(cnfs) ~= "table" then
		return
	end
	settag(cnfs, configs_tag)
	if not cnfs.field then cnfs.field={} end
	if not cnfs.config then cnfs.config={} end
	for i,c in cnfs.config do
		settag(c, config_tag)
	end
--	if not cnfs.default or not cnfs.config[cnfs.default] then
--		local dc = rawget(cnfs.config,1)
--		if dc then cnfs.default = dc.name end
--	end
	print(format("config %q loaded from %q", cnfs.name, fname))
	return cnfs
end

-- Create a "configs" object
--
function configs_create(name, fields)
	if type(name) ~= "string" or type(fields) ~= "table" then
		print("configs_create : bad arguments type")
		return
	end
	local cnfs = { name=name, field={}, config={} }
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
		cnfs.field[i] = {}
		for j,v in f do
			cnfs.field[i][j] = v
		end
	end
	settag(cnfs, configs_tag)
	return cnfs
end

function configs_del(cnfs, config)
	if not configs_check(cnfs) then return end
	if tag(cnfs[config]) == config_tag then
		print(format("configs %q : removing config %q",
						cnfs.name, tostring(config)))
		cnfs[config] = nil
	else
		print(format("configs %q : can not remove config %q",
						cnfs.name, tostring(config)))
	end
end

function configs_add(cnfs, name, values)
	if not configs_check(cnfs) then return end
	if type (name) ~= "string" or strlen(name)<1 then
		print(format("configs %q : %q not a valid name", cnfs.name,
						tostring(name)))
		return
	end

--	print(format("configs_add : %q  name=%q values=%q", cnfs.name, name,
--			tostring(values)))

	local createfrom
	if not values then
		values = "default"
	end
	if type (values) == "string" then
		createfrom = cnfs[values]
	elseif type (values) == "table" then
		createfrom = configs_compatible(cnfs, values)
	end

	if tag(createfrom) ~= config_tag then
		print(format("configs %q : can not create config %q from %q",
				cnfs.name,name,tostring(values)))
		return
	end

	cnfs[name] = createfrom

end

function configs_list(cnfs)
	if not configs_check(cnfs) then return end
	local list = {}
	local i,c
	for i,c in rawget(cnfs,"config") do
		tinsert(list,{ name=i })
	end
	return rawget(cnfs,"default"), list
end


function configs_gettable(cnfs, i)
--	print("configs_gettable : c:"..tostring(cnfs).." i:"..tostring(i))
	if not configs_check(cnfs) then return end
	if type(i) ~= "string" then
		print(format("configs %q : index must be a string", cnfs.name))
		return
	end

	local j,c
	if i == "default" then
		local defname = rawget(cnfs,i)
		if defname then c = rawget(rawget(cnfs,"config"),defname) end
		if c then return c end
		-- default not exist or no more exist : get a new one
		for j,c in rawget(cnfs,"config") do
			print(format("configs %q : set default to %q", cnfs.name,j))
			rawset(cnfs,i,j)
			return c
		end
		print(format("configs %q : no default", cnfs.name))
		return
	end
	return rawget(cnfs,i) or rawget(rawget(cnfs,"config"),i)
end

-- "settable" method
function configs_settable(cnfs,i,v)
--	print("configs_settable : c:"..tostring(cnfs).." i:"..tostring(i)..
--			" v:"..	tostring(v))
	if not configs_check(cnfs) or type(i) ~= "string" then return end

	-- "name" is  reserved names and must be a string
	if i == "name" then
		if type(v) == "string" then
			print(format("configs %q change name to %q", cnfs.name, v))
			rawset(cnfs,i,v)
		end
		return
	end

	-- "default" is reserved and must be a string which is a valid config
	if i == "default" then
		if type(v) == "string" and cnfs.config[v] then
			rawset(cnfs,i,v)
			print(format("configs %q new default %q", cnfs.name, cnfs.default))
		else
			print(format("configs %q : not a valid default %q", cnfs.name,
				tostring(v)))
		end
	end

	-- "field" is reserved and can not be change
	if i == "field" then
		print(format("configs %q : can't change %q ", cnfs.name, i))
		return
	end

	-- Removing a config --
	if not v then
--		print(format("configs %q : remove config %q", cnfs.name, i))
		rawset(cnfs.config, i, nil)
		-- Trying to access "default" field will set it to a valid config in
		-- we just removing it !!!
		-- set to a available config when accessed !!
		local dummy = cnfs.default
		return
	end

	-- Other fields must be "config" object
	if tag(v) ~= config_tag then
		return
	end
	print(format("configs %q : add or modify config %q", cnfs.name, i))
	rawset(cnfs.config, i, dup(v))
end

-- "concat" method
-- 
function configs_concat(a,b)
	local s1,s2
	local t
	t = tag(a)
	if t == configs_tag or t == config_tag then
		s1 = type_dump(a)
	else s1 = tostring(a) end
	t = tag(b)
	if t == configs_tag or t == config_tag then
		s2 = type_dump(b)
	else s2 = tostring(b) end
	return s1..s2
end


settagmethod(configs_tag, "gettable",	configs_gettable)
settagmethod(configs_tag, "settable",	configs_settable)
settagmethod(configs_tag, "concat",		configs_concat)
settagmethod(config_tag, "concat",		configs_concat)


typeA = configs_get_types("integer")
typeB = configs_get_types("string")
typeC = configs_get_types("color")

print("typeA := "..type_dump(typeA))
print("typeB := "..type_dump(typeB))
print("typeC := "..type_dump(typeC))
typeD=dup(typeA)
typeD.notnil=1
print("typeD := "..type_dump(typeD))

conf=configs_create("test-config",
	{
		an_int=typeA, a_str=typeB, a_color=typeC, a_notnil_int=typeD
	}
)

--print("org config = "..conf)
configs_save(conf,"/ram/c")
conf2=configs_load("/ram/c")
--print("loaded config = "..conf2)
conf3=dup(conf)
--print("dup config = "..conf3)
--print("---------------")
print(conf.default)
conf.default=3
conf.default="toto"
conf2.name=3
conf2.name="un vrai nom"
conf2.field=3

--configs_add(conf)
--configs_add(conf,2)
--configs_add(conf,"")
configs_add(conf,"la config ki tue")
configs_add(conf,"la config ki tue","une autre config")
configs_add(conf,"la config vide",{})
print("----------------------------")
print("----------------------------")
configs_add(conf,"enfin-1",{ a_notnil_int=32 })
print("----------------------------")
configs_add(conf,"enfin-2")
configs_add(conf,"enfin-3","enfin-2")

dolib("textlist",1)
def,list = configs_list(conf)
print("liste default:"..def.." "..type_dump(list)) 
fl = textlist_create( {dir=list, lines=1} )
print (fl)
textlist_standalone_run(fl)

print("et finalement = "..conf)
configs_del(conf,"enfin-1")
print("apres del = "..conf)

config_loaded = 1
return config_loaded

