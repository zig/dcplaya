--- @ingroup dcplaya_lua_gui
--- @file    menu.lua
--- @author  benjamin gerard <ben@sashipa.com>
--- @date    2002/10/25
--- @brief   Menu GUI.

menu_loaded=nil
if not dolib("textlist") then return end
if not dolib("style") then return end
if not dolib("box3d") then return end

if not menu_tag then
   menu_tag = newtag()
end

if not menudef_tag then
   menudef_tag = newtag()
end

gui_menu_close_event = gui_menu_close_event or evt_new_code()

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
      border		= 5,
      span              = 1,
   }

   local menu_style = style_get(def.style)
   if tag(menu_style) == style_tag then
      local col0, col1, col2, col3
      local cc = function (a,b) 
		    local i,j,v,r
		    r = {}
		    j = 1
		    for i,v in a do
		       r[j] = v
		       j = j + 1
		    end
		    for i,v in b do
		       r[j] = v
		       j = j + 1
		    end
		    return r
		 end

      -- title bar
      col0 = menu_style:get_color(0,1)
      col3 = menu_style:get_color(0,0.5)
      col1 = (col0+col3) * 0.5
      col2 = col1

      style.titlebar_textcolor   = menu_style:get_color(1,0)

      style.titlebar_topcolor    = menu_style:get_color(0,1)
      style.titlebar_leftcolor   = menu_style:get_color(0,0.7)
      style.titlebar_rightcolor  = menu_style:get_color(0,0.2)
      style.titlebar_bottomcolor = menu_style:get_color(0,0.1)

      col0 = menu_style:get_color(0,0.6)
      col3 = menu_style:get_color(0,0.3)
      col1 = col0 * 0.7 + col3 * 0.3
      col2 = col0 * 0.3 + col3 * 0.7
      style.titlebar_bkgcolor    = { col0,col1,col2,col3 }

      -- menu body 
      local o = 0.3

      style.body_topcolor    = menu_style:get_color(1,o)
      style.body_leftcolor   = menu_style:get_color(0.7,o)
      style.body_rightcolor  = menu_style:get_color(0.2,0.1)
      style.body_bottomcolor = menu_style:get_color(0.1,0)

      col0 = menu_style:get_color(0.6,o)
      col3 = menu_style:get_color(0.3,o)
      col1 = col0 * 0.7 + col3 * 0.3
      col2 = col0 * 0.3 + col3 * 0.7
      style.body_bkgcolor    =  { col0,col1,col2,col3 }

      style.body_filecolor   = menu_style:get_color(1,1)
      style.body_dircolor    = style.body_filecolor
      style.body_curcolor    = cc(menu_style:get_color(0.2,0.3),
				  menu_style:get_color(0.2,0.5))
   end


   -- Menu update (handles fade in / fade out)
   -- -----------
   function menu_update(menu, frametime)
      local oldclose,newclose = (menu.closed or 0),2
      local fl = menu.fl
      if fl then
	 fl:update(frametime)
	 newclose = (fl.closed or 0)
      end

      if newclose ~= oldclose and newclose == 2 then
	 menu.closed = newclose>0 and newclose
	 if menu.owner then
	    evt_send(menu.owner, { key = gui_menu_close_event })
	 end
      end

--       if menu.fade == 0 then return end
--       local a,r,g,b
--       a, r, g, b = dl_get_color(menu.dl)
--       a = a + menu.fade * frametime
--       if a > 1 then
-- 	 a = 1
-- 	 menu.fade  = 0
--       elseif a < 0 then
-- 	 a = 0
-- 	 menu.fade  = 0
-- 	 dl_set_active(menu.dl, 0)
--       end
--       menu:set_color(a, r, g, b)
   end

   -- Menu handle
   -- -----------
   function menu_handle(menu, evt)
      local key = evt.key

      if key == evt_shutdown_event then
	 menu:shutdown()
	 return evt
      end

      if key == gui_focus_event then
	 menu:open()
	 return
      end

      if menu.closed then
	 return evt
      end

      local keyconf,keyright = gui_keyconfirm[key],gui_keyright[key]
      if keyconf or gui_keyselect[key] or keyright then
	 if keyright then
	    local pos = menu.fl and menu.fl:get_pos()
	    local entry = pos and menu.fl.dir[pos]
	    if not entry or entry.size >= 0 then
	       return
	    end
	 end
	 local r = menu:confirm() or 0
	 local confirmed = intop(r,'&',1) == 1
	 local change = intop(r,'&',2) == 2
	 evt = nil
	 if confirmed then
	    if keyconf then
	       menu.root_menu:close(1)
	    end
	    evt = { key = gui_item_confirm_event }
	 end
	 return evt
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
	 menu:close()
	 return
      elseif key == gui_focus_event then
	 print("menu focused")
	 vmu_set_text(fl:get_text())
	 return
      elseif key == gui_unfocus_event then
	 print("menu unfocused")
	 vmu_set_text(nil)
	 return
      end
      return evt
   end

   -- Menu move
   -- ---------
   function menu_move(menu,movx,movy,movz,move_sub)
      if tag(menu) ~= menu_tag or not menu.fl then return end
      local box,z = menu.fl.box, menu.fl.bo2 and menu.fl.bo2[3]
      local x,y = box and box[1], box and box[2]
      menu.fl:set_pos(x and movx and x+movx,
		      y and movy and y+movy,
		      z and movz and z+moz)
      if move_sub and type(menu.sub_menu) == "table" then
	 local i,m
	 for i,m in menu.sub_menu do
	    menu_move(m,movx,movy,movz,1)
	 end
      end
   end

   -- Menu open
   -- ---------
   function menu_open(menu)
      menu.closed = nil
      local fl = menu.fl
      if fl then
	 fl.fade_max = 1
	 fl:open()
      end
      dl_set_active(menu.dl,1)
      gui_new_focus(menu)
      local box = menu.fl and menu.fl.box
      local movx,movy
      if box then
	 movx = (box[1] < 0 and -box[1])
	    or (box[3] > 640 and 640-box[3])
	 movy = (box[2] < 0 and -box[2])
	    or (box[4] > 480 and 480-box[4])
	 if (movx or movy) and tag(menu.root_menu) == menu_tag then
	    menu.root_menu:move(movx,movy,nil,1)
	 end
      end
   end

   -- Menu close
   -- ----------
   function menu_close(menu, close_sub)
      if close_sub then
	 local i,v
	 for i,v in menu.sub_menu do
	    if tag(v) == menu_tag then
	       v:close(1)
	    end
	 end
      end
      menu.closed = 1
      local fl = menu.fl
      if fl then
	 fl.fade_min = 0
	 fl:close()
      end
      if menu.owner then
	 evt_app_insert_last(menu.owner, menu)
      end
   end

   -- Menu set color
   -- --------------
   function menu_set_color(menu, a, r, g, b)
      dl_set_color(menu.dl,a,r,g,b)
--      menu.fl:set_color(a,r,g,b)
   end

   -- Menu draw
   -- ---------

   function menu_draw_border(menu, dl)
      local fl = menu.fl
      local box = { 0, 0, fl.bo2[1], fl.bo2[2] }
      local style = menu.style

      -- Draw menu border
      local b3d = box3d(box, 2,
			style.body_bkgcolor,
			style.body_topcolor,
			style.body_leftcolor,
			style.body_bottomcolor,
			style.body_rightcolor)
      b3d:draw(dl)
   end


   function menu_draw_titlebar(menu,dl)
      local def = menu.def
      if not def.title then return end

      local fl = menu.fl
      local style = menu.style
      local w,w2,h2,yt,w3

      w = fl.bo2[1]
      w2,h2 = dl_measure_text(dl, def.title)
      w3 = w2 + 2*fl.border
      if w3 > w then
	 fl:set_box(nil,nil,w3,nil,nil)
	 w = fl.bo2[1]
      end

      h = h2 + style.span+style.border
      yt = -h --+style.border

      local b3d = box3d({0 , yt, w, yt+h}, 2,
			style.titlebar_bkgcolor,
			style.titlebar_topcolor,
			style.titlebar_leftcolor,
			style.titlebar_bottomcolor,
			style.titlebar_rightcolor)
      b3d:draw(dl)

      -- Draw title 
      col = style.titlebar_textcolor
      if col then
	 dl_draw_text(dl, (w-w2)*0.5, yt+(h-h2)*0.5 , 0.1,
		      col[1],col[2],col[3],col[4], def.title)
      end

      -- Create a new box to include title box.
      menu.box = fl.box + {0, -h, 0, 0 }
   end

   function menu_draw_background(menu, dl)
      -- Default box is the same than textlist one. It will change if 
      -- there is a title, since the box will grow.
      menu.box = menu.fl.box
      menu_draw_titlebar(menu,dl)
      menu_draw_border(menu, dl)
   end

   function menufl_draw_background(fl, dl)
      menu_draw_background(fl.owner, dl)
   end


   function menu_draw(menu)
      dl_clear(menu.dl)
      if menu.fl then
	 menu.fl:draw()
	 dl_sublist(menu.dl,menu.fl.dl)
-- 	 dl_set_active(menu.dl,1)
-- 	 dl_set_active(menu.fl.dl,1)
      end
   end

   -- Menu confirm
   -- ------------
   function menu_confirm(menu)
      local fl = menu.fl
      if not fl then return end
      local idx = fl:get_pos()
      if not idx then return end
      local entry = fl.dir[idx]
      local xentry = fl.dirinfo[idx]
      if not entry then return end

      -- Exec callback for both sub-entry or normal entry
      local cbname = menu.def[idx].cbname
--       dump({cbname=cbname, type=type(menu.def.cb)}, "cb")
      if cbname and type(menu.def.cb) == "table" then
	 local cb = menu.def.cb[cbname]
-- 	 dump(menu.def.cb,"all cb")
	 if type(cb) == "function" then
-- 	    dump({cbname,cb},"exec")
	    cb(menu, idx)
	 else
	    print("[menu_confirm] : wrong callback type:"..type(cb))
	 end
      end

      if (entry.size or 0) == -1 then
	 local subname = menu.def[idx].subname
	 if not subname then return end
	 local m = dl_get_trans(fl.dl)
	 local submenu = menu.sub_menu and menu.sub_menu[subname]
	 
	 if tag(submenu) == menu_tag then
	    submenu:open()
	    evt_app_insert_first(menu, submenu)
	 else
	    local y = (xentry and xentry.y) or 0
	    if not menu.def.sub then return end
	    submenu = menu:create(subname, menu.def.sub[subname],
				  { m[4][1]+fl.bo2[1],
				     m[4][2]+y})
	 end
	 return 2
      else
	 return 1
      end
   end

   -- Menu shutdown
   -- -------------
   function menu_shutdown(menu)
      if not menu then return end

      local owner = menu.owner
      if tag(owner) == menu_tag then
	 owner.sub_menu[menu.name] = nil
      end
      if menu.fl then
	 menu.fl:shutdown()
	 menu.fl = nil
      end
      if menu.dl then
	 dl_set_active(menu.dl)
	 menu.dl = nil
      end
      -- $$$ test : not sure I have right to do that here
      -- menu.owner = nil
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
      move = menu_move,
      confirm = menu_confirm,
      shutdown = menu_shutdown,
      create = menu_create,

      -- Members
      style = style,
      dl = dl_new_list(),
      z = gui_guess_z(owner,z),
      def	= def,
      sub_menu = {},
--      fade = 0,
   }
   settag(menu, menu_tag)

   menu.fl = textlist_create(
			     {	owner=menu,
				pos=box,
				flags=nil,
				dir=menu.def,
				filecolor = menu.style.body_filecolor,
				dircolor  = menu.style.body_dircolor,
				bkgcolor  = nil,
				curcolor  = menu.style.body_curcolor,
				border    = menu.style.border,
				span      = menu.style.span,
			     } )

   menu.fl.fade_spd = 4
   menu.fl.draw_background = menufl_draw_background

--       draw_list         = textlist_draw_list,
--       draw_background   = textlist_draw_background,
--       draw_cursor       = textlist_draw_cursor,
--       draw_entry	= textlist_draw_entry,
--       draw              = textlist_draw,


   if not box then
      textlist_center(menu.fl, 0, 0, 640, 480)
   end

-- $$$ ben : draw will create menu box...
-- menu.box = { menu.fl.box[1], menu.fl.box[2], menu.fl.box[3], menu.fl.box[4] }
--   menu:set_color(1, 1, 1, 1)
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
	    local size, sub, cb, main, icon, substart,subend
	    substart,subend,main,sub =
	       strfind(name,"([%w%s_.{}]*)(>?[%w_]*)")
	    if not substart or not main then
	       print("menu-def creation : invalid string ["..name.."]")
	       return
	    end
-- 	    printf("menu parse all:%q",tostring(name))
 	    printf("menu parse main:%q sub:%q", tostring(main), tostring(sub))

	    substart,subend,icon =
	       strfind(name,"^{([%w%s._]+)}")
	    if icon then
	       main=strsub(main,strlen(icon)+3)
	    end

	    substart,subend,name,cb =
	       strfind(main,"([%w%s_]+)(%b{})",1)

	    if not substart then
	       name = strlen(main)>0 and main
	       cb = nil
	    end
	    if not name then
	       print("menu-def creation : missing menu name");
	       return
	    end
	    -- printf("menu parse name:%q cb:%q",name, tostring(cb))
	    if not cb or strlen(cb) < 3 then
	       cb = nil
	    else
	       cb = strsub(cb, 2, strlen(cb)-1)
	       cb = strlen(cb) >= 1 and cb
	    end
	    if sub and strsub(sub,1,1) == ">" then
	       -- Submenu --
	       size = -1
	       sub = strsub(sub,2)
	       sub = ((strlen(sub) > 0) and sub) or name
	       name = name .. ">"
	    else
	       sub = nil
	       size = 0
	    end
-- 	    printf("final menu parse name:%q cb:%q sub:%q %s",
-- 		   tostring(name), tostring(cb), tostring(sub),
-- 		   (size < 0 and "SUB") or "ENTRY")

	    if type(icon) == "string" then
	       name = '<img src="'..icon..'" w="20">'..name
	    end

	    tinsert(menu,
		    {
		       name=name,
		       size=size,
		       icon=icon,
		       subname=sub,
		       cbname=cb,
		    })
	 end
      else
	 stop = len+1
      end
   end
   settag(menu, menudef_tag)
   return menu
end

function menu_create_defs(def,target,parent)
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

   menu.cb = menu.cb or (parent and parent.cb)

   if type(def) == "table" then
      menu.cb = menu.cb or def.cb
      if type(def.sub) == "table" then
	 local i,v
	 menu.sub = {}
	 for i,v in def.sub do
	    menu.sub[i] = menu_create_defs(v, target, menu)
	    if not menu.sub[i] then
	       print("menu_create_defs : failed to create sub-menu [" ..
		     tostring(i).."]")
	       return
	    end
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

