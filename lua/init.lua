--
-- fundamental lua stuffs
--
-- author : Vincent Penne
--
-- $Id: init.lua,v 1.3 2002-09-24 18:29:42 vincentp Exp $
--


-- do this file only once !
if not init_lua then
init_lua=1


-- simple doshellcommand (reimplemented in shell.lua)
function doshellcommand(string)
	dostring(string)
end




-- command helps handling

shell_help_array = {}
shell_general_commands = {}

function addhelp(fname, help_func, loc)
	if type(fname)=="function" then
		fname=getinfo(fname).name
	end
	if fname then
		shell_help_array[fname] = help_func
		if not loc then
			shell_general_commands[fname] = help_func
		end
	else
		print ("Warning : calling addhelp on a nil fname")
	end
end

function help(fname)
	local h

	if fname then
		h = shell_help_array[fname]
	end
	if h then
		dostring (h)
		return
	end

	local driver
	if fname then
		driver = driver_list[fname]
	end
	if driver then
		print ([[driver:]]..driver.name)
		print ([[description:]]..driver.description)
		print ([[authors:]]..driver.authors)
		print ([[version:]]..format("%x", driver.version))
		print ([[type:]]..format("%d", driver.type))
		h = driver.luacommands
		if h then
			print [[commands are:]]
			local t = { }
			local n = 1
			local i, c
			for i, c in h do
				t[n] = c.name .. [[, ]]
				if c.shortname then
					t[n] = c.shortname .. [[, ]]
				end
				n = n+1
			end
			call (print, t)
		end
		return
	end

	print [[general commands are:]]
	local t = { }
	local n = 1
	local i, v
	for i, v in shell_general_commands do
		local name=i
		t[n] = name .. [[, ]]
		n = n+1
	end
	call (print, t)
		print [[drivers are:]]
	t = { } 
	n = 1
	for i, v in driver_list do
		t[n] = i .. ", "
		n = n+1
	end
	call (print, t)
end


usage=help

addhelp(help, [[print [[help(command_name|driver_name) : show information about a command]]]])
addhelp(addhelp, [[print [[addhelp(command_name, string_to_execute) : add usage information about a command]]]])


-- drivers support

function register_commands(dd, commands, force)

	if not commands then
		return
	end

	local i, c
	for i, c in commands do
		if force or c.registered == 0 then
			print ("Registering new command ", c.name)
			setglobal(c.name, c["function"])
			addhelp(c.name, c.usage, 1)
			if c.shortname then
				setglobal(c.shortname, c["function"])
				addhelp(c.shortname, c.usage, 1)
			end
			c.registered = 1
		end
	end

end

function update_driver_list(list, force)

	local i
	local d

	local n=list.n
	for i=1, n, 1 do

		d = list[i]
		if driver_list[d.name] and driver_list[d.name] ~= d then
			print("Warning : replacing driver '", d.name, "' in list")
		end
		driver_list[d.name] = d
--		print (d.name)

		local commands = d.luacommands
		register_commands(driver_list[d.name], commands, force)

	end

end

function update_all_driver_lists(force)
	driver_list = {}
	update_driver_list(inp_drivers, force)
	update_driver_list(vis_drivers, force)
--	update_driver_list(obj_drivers, force)
	update_driver_list(exe_drivers, force)
end

update_all_driver_lists(1)


function driver_load(...)
	call(%driver_load, arg)
	update_all_driver_lists()
end
dl=driver_load



end -- if not init_lua then
