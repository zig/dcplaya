--- @ingroup dcplaya_lua_gui
--- @file    menu.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/10/25
--- @brief   Menu GUI.

menu_loaded=nil
if not dolib("textlist") then return end

--- @defgroup dcplaya_lua_gui Menu GUI
--- @ingroup  dcplaya_lua_gui
--- 

--- Menu definition object.
--- @ingroup dcplaya_lua_menu_gui
--- struct menudef {
---   string     name;    ///< Menu name.
---   number     n;       ///< Number of menu entry.
---   string     title;   ///< Menu title (or nil is none).
---   menuentry  noname;  ///< Array of menu entry.
--- };

--- Menu entry object.
--- @ingroup dcplaya_lua_menu_gui
--- struct menuentry {
---   string     name;    ///< Name of entry (label).
---   number     size;    ///< -1:for sub-menu entry
---   string     subname; ///< Name of sub-menu (or nil)
--- };

--- Menu object.
--- @ingroup dcplaya_lua_menu_gui
--- struct menu {
--- };

--- Menu default confirm function.
--- @ingroup dcplaya_lua_menu_gui
--- @see textlist::confirm
---
function menu_default_confirm(menu)
	if not menu or not menu.dir or not menu.entries then return end
	local entry = fl.dir[fl.pos+1]
	if entry.size and entry.size==-1 then
		return 2
	else
		return 3
	end
end

function menu_open(menu)
	menu.fade = 4
	dl_set_active(menu.dl,1)
end

function menu_close(menu)
	menu.fade = -4;
end

function menu_set_color(menu, a, r, g, b)
	dl_set_color(menu.dl,a,r,g,b)
	if menu.fl.dl then dl_set_color(menu.fl.dl,a,r,g,b) end
	if menu.fl.cdl then dl_set_color(menu.fl.cdl,a,r,g,b) end
end

function menu_shutdown(menu)
	if not menu then return end
	textlist_shutdown(menu.fl)
	dl_destroy_list(menu.dl)
	local i,v
	for i,v in menu do
		menu[i] = nil
	end
end

--- Create a menu GUI application.
--- @ingroup dcplaya_lua_menu_gui
---
function menu(owner, def, box)
	local menu
	local x1,y1,x2,y2

	if not owner then owner = evt_desktop_app end
	if not def then return end
	if not box then
		x1 = 100
		y1 = 100
		x2 = 300
		y2 = 300
	else
		x1 = box[1]
		y1 = box[2]
		x2 = box[3]
		y2 = box[4]
	end

-- ----------------
-- MENU APPLICATION
-- ----------------

	-- menu update function handles fade in / fade out
	function update(menu, frametime)
		if menu.fade == 0 then return end
		local a,r,g,b
		a, r, g, b = dl_get_color(menu.dl)
		a = a + menu.fade * frametime
		if a > 1 then
			a = 1
			menu.fade  = 0
		elseif a < 0 then
			a = 0
			menu.fade  = 0
			dl_set_active(menu.dl, 0)
		end
		menu_set_color(menu, a, r, g, b)
	end

	function handle(menu, evt)
		local key = evt.key

		if key == evt_shutdown_event then
			menu_shutdown(menu)
			return evt
		elseif gui_keyconfirm[key] then
			return
		elseif gui_keycancel[key] then
			evt_shutdown_app(menu)
			return
		elseif key == gui_item_change_event then
			return
		elseif gui_keyup[key] then
			textlist_movecursor(menu.fl,-1)
			return
		elseif gui_keydown[key] then
			textlist_movecursor(menu.fl,1)
			return
		elseif gui_keyleft[key] then
			return
		elseif gui_keyright[key] then
			return
		end
		return evt
	end

	menu = {
		name = name,
		version = 1.0,
		handle = handle,
		update = update,
		dl = dl_new_list(),
		z = gui_orphanguess_z(nil),
		def	= menu_create_menulist(def),
	}

-- --------
-- TEXTLIST
-- --------
	function confirm(fl)
		if not fl or not fl.dir or fl.n < 1 then return end
		local entry = fl.dir[fl.pos+1]
		if not entry then return end
		if entry.size and entry.size==-1 then
			local action
			if not action then
				return
			end
			return 2
		else
			return 3
		end
	end
	
	menu.fl = textlist_create(
				{	pos={x1,y1},
					confirm=confirm,
					flags=nil,
					dir=menu.def,
					filecolor = { 1, 0, 0, 0 },
					dircolor  = { 1, 0, 0, .4 },
					bkgcolor  = { 1, 0.7, 0.7, 0.7,  1, 0.0, 0.3, 0.3 },
					curcolor  = { 1, 1, 1, 1,         1, 0.1, 0.4, 0.5 },
					border    = 5,
				} )

	menu.def.name = "Menu"

	if menu.def.name then
		local w,h
		w,h = dl_measure_text(menu.dl, menu.def.name)
		h = h + menu.fl.border + menu.fl.span
		dl_set_trans(menu.dl, mat_trans(0,-h,0) * menu.fl.mtx)
		dl_draw_box(menu.dl, 0 , 0, menu.fl.bo2[1], h, 0,
					1, 0, 0, 0.2, 
					1, 0, 0, 0.7, 2) 
		dl_draw_text(menu.dl,
					 (menu.fl.bo2[1] - w) * 0.5, menu.fl.border, 0.1,
					1, 1, 1, 1,
					menu.def.name)
	end
	menu_set_color(menu, 0, 1, 1, 1)
	menu_open(menu)

	evt_app_insert_first(owner, menu)
	return menu

end

--- Create a menudef from a string.
--- @ingroup dcplaya_lua_menu_gui
---
---  @param   menustr  Menu creation string.
---  @return  menudef object
---  @retval  nil      Error
---
--- menu creation string syntax:
--- menustr := [:title:]entry[,entry]*
---  - @b title (^:)* : An optional string with no ':' char.
---  - @b entry (^,)*[>[(^,)*]] :
---          A string with no ',' char optionally followed by a '>' char for
---          a submenu entry, optionally followed by the name of the sub-menu.
---
function menu_create_menulist(menustr)
	local start, stop, menu, title
	local len
	if type(menustr) ~= "string" then
		return
	end
	menu = {}

	len = strlen(menustr)

	-- Get title if any.
	start, stop, title = strfind(menustr,"^:(.*):")
	stop = 1
	if title then
		menu.title = title
		stop = start+strlen(title)+2
	end

	-- Parse entries
	while stop <= len do
		-- Get entries
		start, stop, name = strfind(menustr,"([^,]*)",stop)
		if name and strlen(name) > 0 then
			stop = stop+2
			if name == "-" then
				-- Separator line --
				if not menu.separator then menu.separator = {} end
				tinsert(menu.separator, getn(menu))
			else
				local size = 0
				local sub = nil
				local substart,subend
				substart,subend = strfind(name,">",1,1)
				if substart then
					-- Submenu --
					size = -1
					sub = strsub(name,substart+1)
					name = strsub(name,1,substart)
					if strlen(sub) < 1 then sub = nil end
				end
				tinsert(menu, {name=name, size=size, subname=sub})
			end
		else
			stop = len+1
		end
	end
	return menu
end

--menu = create_menulist(":un autre menu:entry 1>,entry 2,-,entry 3>zob,entry 4,")
--dump(menu,menu.title)

dial = nil
if not nil then
	print("Run test (y/n) ?")
	c = getchar()
	if c == 121 then
		dial = menu(nil,":test menu:info,kill,hide,more>")
	end
	function k() if dial then menu_shutdown(dial) end end
--	k()
end


menu_loaded=1
return menu_loaded

