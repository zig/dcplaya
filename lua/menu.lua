--- @ingroup dcplaya_lua_gui
--- @file    menu.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/10/25
--- @brief   Menu GUI.

menu_loaded=nil
if not dolib("textlist") then return end

if not menu_tag then
   menu_tag = newtag()
end

if not menudef_tag then
   menudef_tag = newtag()
end

--- @defgroup dcplaya_lua_menu_gui Menu GUI
--- @ingroup  dcplaya_lua_gui
--- 

--- Menu definition object.
--- @ingroup dcplaya_lua_menu_gui
--- struct menudef : applcation {
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
---
--- struct menu : application {
---   open();	           ///< Show/active menu
---   close();             ///< Hide/desactive menu
---   set_color();         ///< Set global color
---   draw();              ///< Build display lists
---   confirm();           ///< Menu confirm callback
---   shutdown();          ///< Shutdown menu
---   create();            ///< Create a new menu
---
---   style        style;      ///< Menu style
---   display_list dl;         ///< Menu display list
---   menudef      def;        ///< menu definition
---   menu         sub_menu[]; ///< Created sub-menu (indexed by name)
---   number       fade;       ///< Current fade step.
--- };

--- Create a menu application.
--- @internal
---
--- @param  name  menu name. If nil was given menu title is used as menu name. 
--- @param  def   menu definition.
--- @param  box   { x1, y1, x2, y2 }
---
--- @return menu aplication
--- @retval nil Error.
---
function menu_create(owner, name, def, box, x1, y1)
   local menu
   local x2,y2,z

   if not owner or tag(def) ~= menudef_tag then return end
   if not name then name = def.title end

   x1 = x1 or 0
   y1 = y1 or 0
   if box then
      x1 = box[1]
      y1 = box[2]
      x2 = box[3]
      y2 = box[4]
      z  = box[5]
   end

   -- ----------------
   -- MENU APPLICATION
   -- ----------------

   -- Menu default style
   -- ------------------
   local style = {
      bkg_color		= { 1, 0.7, 0.7, 0.7,  1, 0.3, 0.3, 0.3 },
      title_color	= { 1,1,1,1 },
      titlebar_color	= { 1, 0, 0, 0.2,  1, 0, 0, 0.7, 2},
      titlebar_type	= 2,
      border		= 5,
      span              = 1,
      file_color	= { 1, 0, 0, 0 },
      dir_color		= { 1, 0, 0, .4 },
      cur_color		= { 1, 1, 1, 1,  1, 0.1, 0.4, 0.5 },
   }

   -- Menu update (handles fade in / fade out)
   -- -----------
   function menu_update(menu, frametime)
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
      menu:set_color(a, r, g, b)
   end

   -- Menu handle
   -- -----------
   function menu_handle(menu, evt)
      local key = evt.key

      if key == evt_shutdown_event then
	 menu:shutdown()
	 return evt
      end

      if menu.closed then
	 return evt
      end

      if gui_keyconfirm[key] or gui_keyselect[key] then
	 local r = menu:confirm()
	 if gui_keyconfirm[key] then
	    menu:close()
	 end
	 return r
      elseif gui_keycancel[key] then
	 evt_shutdown_app(menu.root_menu)
	 return
      elseif key == gui_item_change_event then
	 return
      elseif gui_keyup[key] then
	 menu.fl:move_cursor(-1)
	 return
      elseif gui_keydown[key] then
	 menu.fl:move_cursor(1)
	 return
      elseif gui_keyleft[key] then
--	 if tag(menu.owner) == menu_tag then
	    menu:close()
--	 end
	 return
      elseif gui_keyright[key] then
	 local fl = menu.fl
	 if fl.dir[fl.pos+1].size < 0 then
	    return menu:confirm()
	 end
	 return
      end
      return evt
   end

   -- Menu open
   -- ---------
   function menu_open(menu)
      menu.fade = 4
      menu.closed = nil
      dl_set_active(menu.dl,1)
      gui_new_focus(menu)
   end

   -- Menu close
   -- ----------
   function menu_close(menu)
      menu.closed = 1
      menu.fade = -4;
      if menu.owner then
	 evt_app_insert_last(menu.owner, menu)
      end
   end

   -- Menu set color
   -- --------------
   function menu_set_color(menu, a, r, g, b)
      dl_set_color(menu.dl,a,r,g,b)
      menu.fl:set_color(a,r,g,b)
   end

   -- Menu draw
   -- ---------
   function menu_draw(menu)
      local fl  = menu.fl

      local dl  = fl.dl
      dl_clear(dl)

      -- Draw textlist $$$ ben : Add this here but it does not seem to work !
      fl:draw()

      local dl  = fl.dl
      local def = menu.def
      local style = menu.style
      local w,h,col
      local active = dl_set_active(dl,0)
--      dl_clear(dl)
      w = fl.bo2[1]

      -- Default box is the same than textlist one. It will change if 
      -- there is a title, since the box will grow.
      menu.box = fl.box

      -- Draw title bar
      h = 0
      if def.title then
	 local w2,h2,yt,w3
	 w2,h2 = dl_measure_text(dl, def.title)
	 w3 = w2 + 2*fl.border
	 if w3 > w then
	    fl:set_box(nil,nil,w3,nil,nil)
	    w = fl.bo2[1]
	 end
	 h = h2 + style.span+style.border
	 def.title_h = h
	 yt = -h+style.border
	 col = style.titlebar_color
	 if col then
	    dl_draw_box(dl, 0 , yt, w, yt+h, 0,
			col[1],col[2],col[3],col[4],
			col[5],col[6],col[7],col[8],
			style.titlebar_type)
	 end
	 col = style.title_color
	 if col then
	    dl_draw_text(dl, (w-w2)*0.5, yt+(h-h2)*0.5 , 0.1,
			 col[1],col[2],col[3],col[4], def.title)
	 end
	 -- Create a new box to include title box.
	 menu.box = fl.box + {0, -h, 0, 0 }
      end


      -- Draw menu background
      col = style.bkg_color
      if col then
	 local bkgtype = style.bkg_type
	 local i,max
	 max=fl.pos+fl.dir.n
	 if max > fl.dir.n then max = fl.dir.n end
	 for i=fl.pos+1, max do
	    local e = fl.dir[i]
	    local info = fl.dirinfo[i]
	    dl_draw_box(dl, 0, info.y+5, w, info.y+5+info.h, -0.1,
			col[1],col[2],col[3],col[4],
			col[5],col[6],col[7],col[8],
			bkgtype)
	 end
      end


--      dl_set_clipping(dl,0,0,w,0)
--      dl_set_trans(dl, fl.mtx)
      dl_set_active(dl,active)
   end

   -- Menu confirm
   -- ------------
   function menu_confirm(menu)
      local fl = menu.fl
      local idx = fl.pos+1
      local entry = fl.dir[idx]
      local info = fl.dirinfo[idx]
      if entry.size and entry.size==-1 then
	 local subname = menu.def[idx].subname
	 local m = dl_get_trans(fl.dl)
	 local submenu = menu.sub_menu[subname]
	 
	 if tag(submenu) == menu_tag then
	    --				print(format("menu %q is a sub of %q",subname,menu.name))
	    submenu:open()
	    evt_app_insert_first(menu, submenu)
	 else
	    --				print(format("menu %q is NOT a sub of %q",subname,menu.name))
	    --				dump(menu.sub_menu, format("%s BEFORE", menu.name))

	    local y = info.y
	    submenu = menu:create(subname, menu.def.sub[subname],
				  { m[4][1]+fl.bo2[1], m[4][2]+y-menu.style.border})

	    --				dump(menu.sub_menu, format("%s AFTER", menu.name))

	 end
      else
	 local cbname = menu.def[idx].cbname
	 if cbname and type(menu.def.cb) == "table" then
	    local cb = menu.def.cb[cbname]
	    if cb then
	       cb(menu, idx)
	    end
	 end
      end
      return
   end

   -- Menu shutdown
   -- -------------
   function menu_shutdown(menu)
      if not menu then return end

      local owner = menu.owner
      if tag(owner) == menu_tag then
	 owner.sub_menu[menu.name] = nil
      end
      menu.fl:shutdown()
      --dl_destroy_list(menu.dl)
       dl_set_active(menu.dl)
      menu.dl = nil
   end

   menu = {
      -- Application
      name = name,
      version = 1.0,
      handle = menu_handle,
      update = menu_update,

      -- Methods
      open = menu_open,
      close = menu_close,
      set_color = menu_set_color,
      draw = menu_draw,
      confirm = menu_confirm,
      shutdown = menu_shutdown,
      create = menu_create,

      -- Members
      style = style,
      dl = dl_new_list(64 * (def.n or 0) + 1024),
      z = gui_guess_z(owner,z),
      def	= def,
      sub_menu = {},
      fade = 0,
   }
   settag(menu, menu_tag)

   menu.fl = textlist_create(
			     {	owner=menu,
				pos=box,
				flags=nil,
				dir=menu.def,
				filecolor = menu.style.file_color,
				dircolor  = menu.style.dir_color,
				bkgcolor  = nil,
				curcolor  = menu.style.cur_color,
				border    = menu.style.border,
				span      = menu.style.span,
			     } )

   if not box then
      textlist_center(menu.fl, 0, 0, 640, 480)
   end

-- $$$ ben : draw will create menu box...
-- menu.box = { menu.fl.box[1], menu.fl.box[2], menu.fl.box[3], menu.fl.box[4] }

   menu:set_color(0, 1, 1, 1)
   menu:draw()

   if tag(owner) == menu_tag then
      --		print(format("owner=%q root=%q me=%q",
      --			owner.name, owner.root_menu.name,menu.name))
      menu.root_menu = owner.root_menu
      owner.sub_menu[menu.name] = menu
   else
      --		print(format("owner is not a menu : %q becoming root", menu.name))
      menu.root_menu = menu
   end

   menu:open()

   evt_app_insert_first(owner, menu)
   return menu
end

--- Create a menu GUI application.
--- @ingroup dcplaya_lua_menu_gui
---
function gui_menu(owner, name, def, box)
   if not owner then owner = evt_desktop_app end
   return menu_create(owner, name, def, box)
end


function menu_merge_def(def1,def2)
   local def
   if tag(def1) ~= menudef_tag then
      print("menu_merge_def : 1st parameter is not a menudef.")
      return
   end
   if not def2 then return def1 end
   if tag(def2) ~= menudef_tag then
      print("menu_merge_def : 2nd parm is not a menudef ["..type(def2).."]")
      return
   end

   def = { n=0 }
   local i,v

   -- Copy first def.
   for i,v in def1 do
      if type(i) == "number" then
-- 	 print("Insert "..tostring(v))
	 tinsert(def,v)
      else
-- 	 print("Copy "..i.." : "..tostring(v))
	 def[i] = v
      end
   end

   -- Merge second def.
   for i,v in def2 do
      if type(i) == "number" then
-- 	 print("merging #"..i)
	 tinsert(def,v);
      elseif type(i) == "string" then
	 if i == "title" then
	    def.title = def.title or v
	 elseif i == "cb" or i == "sub" then
-- 	    print("merging ["..i.."]")
	    if not def[i] then def[i] = {} end
	    
	    local j,w
	    for j,w in v do
	       if not def[i][j] then
		  def[i][j] = w
	       else
		  print("menu_merge_def : conflicting ["..i.."."..j.."]")
		  return
	       end
	    end
	 else
	    if def[i] and def[i] ~= v then
	       print("menu_merge_def : lost ["..i.." := "..tostring(v).."]")
	    else
	       def[i] = v
	    end
	 end
      else
	 print("menu_merge_def : dunno what to do with ["..type(i).."]")
      end
   end
     
   settag(def,menudef_tag)
   return def
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
function menu_create_def(menustr)
   local start, stop, menu, title
   local len

   if tag(menustr) == menudef_tag then
      -- $$$
      print("menu_create_def : already a menudef")
      return menustr
   elseif type(menustr) ~= "string" then
      return
   end

   menu = { }
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
	    local cb
	    local substart,subend
	    substart,subend = strfind(name,">",1,1)
	    if substart then
	       -- Submenu --
	       size = -1
	       sub = strsub(name,substart+1)
	       name = strsub(name,1,substart)
	       if strlen(sub) < 1 then sub = nil end
	    else
	       substart,subend = strfind(name,"%b{}",1)
	       if substart then
		  -- Submenu --
		  cb = strsub(name,substart+1, subend-1)
		  name = strsub(name,1,substart-1)
		  if strlen(cb) < 1 then cb = nil end
	       end
	    end
	    tinsert(menu, {name=name, size=size, subname=sub, cbname=cb})
	 end
      else
	 stop = len+1
      end
   end
   settag(menu, menudef_tag)
   return menu
end

function menu_create_defs(def,target)
   local menu

--    print("menu_create_defs : "..type(def))

   if not def then
      return -- Cause no error message
   elseif type(def) == "string" then
      menu = menu_create_def(def)
   elseif type(def) == "function" then
      menu = def(target)
   elseif type(def) == "table" then
      if type(def.creator) == "function" then
	 menu = def.creator(def, target)
      elseif def.root then
	 menu = menu_create_def(def.root)
      end
   end

--    print("menu_create_defs (intermediat) := "..tostring(menu))

   if tag(menu) ~= menudef_tag then
      print("menu_create_defs : not a menudef")
      return
   end

   if type(def) == "table" then
      menu.cb = menu.cb or def.cb
      if type(def.sub) == "table" then
	 local i,v
	 menu.sub = {}
	 for i,v in def.sub do
	    menu.sub[i] = menu_create_defs(v, target)
	    if not menu.sub[i] then return end
	 end
      end
   end

--    print("menu_create_defs := "..tostring(menu))

   return menu
end

--menu = create_menulist(":un autre menu:entry 1>,entry 2,-,entry 3>zob,entry 4,")
--dump(menu,menu.title)

if nil then
   dial = nil
   print("Run test (y/n) ?")
   c = getchar()
   if c == 121 then
      local def = 
	 {
	 root=":test:info{info},kill >kill,hide,more >more",
	 cb = {
	    info = function(menu) print ("INFO") end
	 },
	 sub = {
	    kill = ":kill:destroy,obliteration",
	    more = {
	       root = ":more:more 1,more 2,more 3 >toto, more 4{more}",
	       cb = {
		  more = function(menu) print("MORE") end
	       },
	       sub  = {
		  toto=":toto:toto 1,toto 2,toto 3"
	       }
	    }
	 }
      }
      dial = gui_menu(nil,"menu1", menu_create_defs(def))
   end
   function k() if dial then dial:shutdown() end end
   --	k()
end


menu_loaded=1
return menu_loaded

