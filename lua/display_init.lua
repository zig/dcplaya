if not init_display_driver then

plug_display = home.."plugins/exe/display/display.lez"

-- check that display driver is available
function check_display_driver()
	if not dl_new_list then
		driver_load(plug_display)
	end
	if not dl_new_list then
		print [[display driver not found]]
		return nil
	end

	return 1
end

check_display_driver()


function list_expand(from, to)
	local r = {}
	local i, v
	for i, v in from do

		if type(v)=="table" then
			list_expand(v, to)
		else
			tinsert(to, v)
		end

	end
end

-- reimplement with automatic list expansion
local re = function(name)
	local f = getglobal(name)
	if not f then
		return
	end
	local n = function(...)

		local list = {}
		list_expand(arg, list)

		call(%f, list)

	end
	print ("reimplemented for list expansion : ", name)
	setglobal(name, n)
end

-- reimplement some dl function with automatic list expansion
re "dl_draw_box"
re "dl_draw_text"


-- set it at the end so that it is not set if something failed before !
init_display_driver = 1

display_init_loaded = 1

end -- if not init_display_driver then
