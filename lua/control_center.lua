--- @ingroup  dcplaya_lua_app
--- @file     control_center.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    control center application.
---
--- $Id: control_center.lua,v 1.12 2003-03-08 13:54:25 ben Exp $
---

--- @defgroup dcplaya_lua_cc_app Control center application
--- @ingroup dcplaya_lua_app
---
---  @par Control Center introduction
---
---   The control center application is a multi purpose application. It
---   is used to retrieve dcplaya component information, to configure plugins
---   and many things.
---

--
--- @name Control center functions.
--- @ingroup dcplaya_lua_cc_app 
--- @{
--

control_center_loaded = nil

if not dolib("gui") then return end
if not dolib("menu") then return end
if not dolib("volume_control") then return end
if not dolib("plugin_info") then return end

--
--- Volume menu callback.
---
---   The control_center_volume() function creates a volume control
---   application.
---
--- @see dcplaya_lua_vc_app
--- @internal
--
function control_center_volume(menu)
   local cc = menu.target
   volume_control_create(); -- $$$ do not works with cc has owner
end

--
--- About menu callback.
---
---   The control_center_about() function creates and handle the about dialog.
---
--- @internal
--
function control_center_about(menu)
   local cc = menu.target

   local macro =
      '<macro macro-name="tfont" macro-cmd="font" color="#FFE080" size="20"><macro macro-name="url" macro-cmd="font" color="#80E0FF" size="13"><macro macro-name="normal" macro-cmd="font" color="#e0e080" size="16"><macro macro-name="author" macro-cmd="font" color="#ff8080" size="18"><macro macro-name="pspace" macro-cmd="vspace" h="12">'

   local text = macro .. '<center><tfont><img name="dcplaya64" src="dcplaya.tga" h="64"><br>dcplaya'
   if __DEBUG then
      text = text .. ' (debug)'
   end
   if __VERSION then
      text = text .. ' version ' .. __VERSION
   end

   text = text .. '<p vspace="2"><pspace><normal>(c)2002-2003<br><author>Benjamin<img src="le_ben.jpg" name="ben_mini" h="48">Gerard<br><normal>and<br><author>Vincent<img src="le_zig.jpg" name="zig_mini" h="48">Penne'

   if __URL then
      text = text
	 .. '<p><pspace><normal>Visite dcplaya official website<br><url>' .. __URL
   end

   --       local di = driver_info()
   --       if type(di) == "table" then
   -- 	 local k,v
   -- 	 local convert = {
   -- 	    inp="input",
   -- 	    vis="visual",
   -- 	    img="image",
   -- 	    exe="executable",
   -- 	    obj="3D object" }
   -- 	 for k,v in di do
   -- 	    if type(v) == "table" and getn(v) > 0 then
   -- 	       local type = convert[k] or k;
   -- 	       text = text . '<p><center><dfont>' .. type .. 'plugins'
   -- 	       local pname, p
   -- 	       for pname,p in v do
   -- 		  text = text .. '<p><left>'
   -- 	       end

   gui_ask(text,
	   {'<center><img name="stock_button_cancel" src="stock_button_cancel.tga ">close'},
	   400,'About dcplaya')
end

--
--- Create an icon sprite.
--- @internal
--
function control_center_create_sprite(cc, name, src, w, h, u1, v2, u2, v2, rotate)
   local spr = sprite_get(name)
   if spr then
      printf("spr %q %q exist",name,src)
      return spr
   end
   printf("spr %q %q not exist",name,src)
   
   local tex = tex_get(src)
      or tex_new(home.."lua/rsc/icons/"..src)

   if not tex then
      printf("spr %q %q can not create texture",name,src)
      return
   end

   local info = tex_info(tex)
   local info_w, info_h = info.w, info.h
   local orig_w, orig_h = info.orig_w, info.orig_h

   u1 = u1 or 0
   v1 = v1 or 0
   u2 = u2 or orig_w/info_w
   v2 = v2 or orig_h/info_h

   if w then
      if not h then
	 h = orig_h * w / orig_w
      end
   elseif h then
      if not w then
	 w = orig_w * h / orig_h
      end
   else
      w = orig_w
      h = orig_h
   end

   spr = sprite(name, 0, 0, w, h,
		u1, v1, u2, v2, tex, rotate)

   return spr
end

--
--- Create all icon sprites. 
--- @internal
--
function control_center_create_sprites(cc)
   control_center_create_sprite(cc, "cc_dcp", "dcplaya.tga", 32)
   control_center_create_sprite(cc, "cc_vmu", "vmu32.tga",32)
   control_center_create_sprite(cc, "cc_vol", "volume2.tga", 32)
   --				   24, 24) --, 0 , 0, 8/64, 48/64)
end

--
--- Menu creator.
---
--- @return a menu definition
--- @see dcplaya_lua_menu_app
--- @internal
--
function control_center_menucreator(target)
   control_center_create_sprites(cc)

   local root = ":" .. target.name .. ":" .. 
      '{cc_dcp}about{about},{cc_vol}volume{volume},{cc_vmu}vmu >vmu,plugins >plugins'

   local cb = {
      about = control_center_about,
      volume = control_center_volume,
      setvmuvis = function (menu) 
		     local cc = menu.root_menu.target
		     local pos = menu.fl:get_pos()
		     if pos then pos = pos - 1 end
		     vmu_set_visual(pos)
		  end,
      plug_help = function (menu) 
		     local cc = menu.root_menu.target
		     local fname = home .. "lua/rsc/text/plugins.txt"
		     gui_file_viewer(nil, fname, nil, "plugins")
		  end,
      set_visual = function (menu) 
		      local name=menu.fl:get_text()
		      if name then
			 name = (name ~= "none") and name
			 print(name)
			 set_visual(name)
		      end
		   end,
      plug_info  = function (menu)
		      local title = menu.def.title or "*"
		      local name = menu.fl:get_text()
		      if name then
			 plugin_viewer(nil,title.."/"..name)
		      end
		   end,
   }

   -- Read available driver type
   local drlist = get_driver_lists()
   if type(drlist) ~= "table" then drlist = {} end

   local plugins = {
      root = ":plugins:",
      sub  = {}
   }
   local plug_info
   local cur_vis = set_visual() or "none"

   -- vis driver type exist ? (should be always the case)
   if drlist.vis then
      plugins.root = plugins.root .. "set visual >set_visual"
      plugins.sub.set_visual = ":visual:none{set_visual}"
      local i = drlist.vis.n
      for i=1, drlist.vis.n do
	 local vis = drlist.vis[i]
	 if vis and type(vis.name) == "string" then
	    plugins.sub.set_visual = plugins.sub.set_visual
	       .. "," .. vis.name .. "{set_visual}"
	 end
      end
   end

   -- Fill the info submenu with all available driver types.
   local k,v
   for k,v in drlist do
      local subname = "inf_" .. k
      if not plug_info then
	 plug_info = {
	    root = ":info:help{plug_help}",
	    sub = {},
	 }
	 plugins.sub.info = plug_info
	 plugins.root = plugins.root ..
	    ((plugins.sub.set_visual and ",") or "")
	 plugins.root = plugins.root .. "info >info"
      end
      plug_info.root = plug_info.root .. "," .. k .. ">" .. subname
      local info_root = nil

      local j
      for j=1, v.n do
	 local plg = v[j]
	 if plg then
	    -- 	       info_root = ((info_root and (info_root .. ","))
	    -- 			    or (":"..k..":")) .. plg.name .. "{plug_info}"
	    info_root = (info_root or (":"..k..":*{plug_info}"))
	       .. "," .. plg.name .. "{plug_info}"
	 end
      end
      if info_root then
	 plug_info.sub[subname] = { root = info_root }
      end
   end

   local def = {
      root=root,
      cb = cb,
      sub = {
	 vmu = {
	    root = ":vmu:visual >vmu_visual,save",
	    sub = {
	       vmu_visual = ":visual:none{setvmuvis},scope{setvmuvis},fft{setvmuvis},band{setvmuvis}"
	    }
	 },
	 plugins = plugins,
      }
   }
   return menu_create_defs(def , target)
end


--
--- Create a control center application.
---
--- @param owner owner application (default to desktop)
--- @param name  application name (default to "control center")
--- @return application table
--- @retval nil on error
--
function control_center_create(owner, name)
   owner = owner or evt_desktop_app
   name = name or "control center"
   local z



   local cc = {
      -- Application
      name = name,
      version = 1.0,
      handle = nil,
      update = nil,

      icon_name = "control-center",
      
      -- Members
      z = gui_guess_z(owner,z),
      fade = 0,
      dl = dl_new_list(256, 1),
      mainmenu_def = control_center_menucreator
   }

   evt_app_insert_last(owner, cc)
   return cc
end

if control_center then
   evt_shutdown_app(control_center)
end

control_center = control_center_create()

--
--- Kill a control center application.
---
---   The control_center_kill() function kill the given application by
---   calling sending the evt_shutdown_app() funciton. If the given
---   application is nil or control_center the default control center
---   (control_center) is killed and the global control_center is set
---   to nil.
---
--- @param  cc  application to kill (default to control_center)
--
function control_center_kill(cc)
   cc = cc or control_center
   if cc then
      evt_shutdown_app(cc)
      if cc == control_center then control_center = nil end
   end
end

-- Load application icon
for k,v in { "dcplaya", "vmu32", "volume", "control-center" } do

   local tex = tex_get(v) or
		    tex_new(home .. "lua/rsc/icons/" .. v .. ".tga")
end

--
--- @}
--

control_center_loaded = 1
return control_center_loaded
