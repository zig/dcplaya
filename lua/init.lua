--
-- fundamental lua stuffs
--
-- author : Vincent Penne
--
-- $Id: init.lua,v 1.2 2002-09-24 13:47:04 vincentp Exp $
--


-- do this file only once !
if not init_lua then
init_lua=1


-- drivers handling




-- command helps handling
shell_help_array = {}

function addhelp(fname, help_func)
	shell_help_array[fname] = help_func
end

function help(fname)
	local h = nil
	if fname then
		if type(fname)==[[string]] then
		fname = getglobal(fname)
	end
		h = shell_help_array[fname]
	end
	if h then
		dostring (h)
	else
		print [[commands are:]]
		local t = { } local n = 1
		for i, v in shell_help_array do
			local name=getinfo(i).name
			if not name then name = [[(?)]] end
			t[n] = name .. [[, ]]
			n = n+1
		end
		call (print, t)
--		for i, v in shell_help_array do
--			print ([[  ]], getinfo(i).name, [[,]])
--		end
	end
end

-- simple doshellcommand (reimplemented in shell.lua)
function doshellcommand(string)
	dostring(string)
end

usage=help

addhelp(help, [[print [[help(command) : show information about a command\n]]]])
addhelp(addhelp, [[print [[addhelp(command, string_to_execute) : add usage information about a command\n]]]])


end -- if not init_lua then
