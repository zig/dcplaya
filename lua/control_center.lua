--- @ingroup  dcplaya_lua_cc_app
--- @file     control_center.lua
--- @author   benjamin gerard
--- @date     2002
--- @brief    control center application.
---
--- $Id: control_center.lua,v 1.31 2004-07-31 22:55:18 vincentp Exp $
---

--- @defgroup  dcplaya_lua_cc_app  Control Center
--- @ingroup   dcplaya_lua_app
--- @brief     control center application
---
---   The control center application is a multi-purpose application. It
---   is used to retrieve dcplaya component information, to configure plugins
---   and many things.
---
--- @author   benjamin gerard
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
if not dolib("vmu_init") then return end
if not dolib("fileselector") then return end

control_center_close_button = '<center><img name="stock_button_cancel" src="stock_button_cancel.tga ">close'

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

   gui_ask(text,
	   {control_center_close_button},
	   400,'About dcplaya')
end

--
--- Create all icon sprites. 
--- @internal
--
function control_center_create_sprites(cc)
   menu_create_sprite("dcp", "dcplaya.tga", 32)
   menu_create_sprite("vmu", "vmu32.tga",32)
   menu_create_sprite("vol", "volume2.tga", 32)
   menu_create_sprite("kbd", "keyboard.tga", 32)
   menu_yesno_menu(1,"","")
end

function vmu_save_confirm(vmu)
   if type(vmu) ~= "string" then return end
   local r = vmu_confirm_write(vmu)
   if r and r == 1 then
      if not vmu_save_file(vmu, "/ram/dcplaya") then
	 gui_ask('<img name="vmu" w="48">Failed to write dcplaya file : <br><p><vspace h="8"><font color="#FF0000">'..vmu, { control_center_close_button }, 400, "Write failure")
      elseif not vmu_never_confirm_write then
	 gui_ask('<img name="vmu" w="48">Successfully write dcplaya file : <br><p><vspace h="8"><font color="#00FF00">'..vmu, { control_center_close_button }, 400, "Write success")
	 return 1
      end
   end
end

function vmu_save_as(vmu)
   if type(vmu) ~= "string" then vmu = nil end
   local dir = vmu_list()
   if not dir then return end

   local path, leaf
   if vmu then
      if strsub(vmu,1,1) == "/" then
	 path,leaf = get_path_and_leaf(vmu)
      else
	 path,leaf = get_path_and_leaf(vmu)
	 path = nil
      end
   end
   path = path or "/vmu"

   local choice
   local fs = fileselector("Choose save file", path, leaf)
   choice = fs and evt_run_standalone(fs)
   if choice then
      return vmu_save_confirm(choice)
   end
end

function vmu_save(vmu, alternate)
   vmu = (type(vmu) == "string" and vmu) or vmu_file()
   if not vmu then
      vmu = vmu_choose_file()
      if not vmu and alternate then
	 return vmu_save_as(type(alternate) == "string" and alternate)
      end
   end
   return vmu_save_confirm(vmu)
end

--
--- Menu creator.
---
--- @return a menu definition
--- @see dcplaya_lua_menu_app
--- @internal
--
function control_center_menucreator(target)
   local cc = target
   control_center_create_sprites(cc)

   local root = ":" .. target.name .. ":" .. 
      '{menu_dcp}about{about},{menu_vol}volume{volume},{menu_vmu}vmu >vmu,plugins >plugins,{menu_kbd}keyboard >keyboard,{menu_net}network{network}'


   local cb = {
      about = control_center_about,
      volume = control_center_volume,
      network = net_connect_dialog,
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

      vmu_load  = function (menu)
		     if not ram_path then ramdisk_init() end
		     if not ram_path then return end
		     local vmufile = vmu_file()
		     local result
		     if vmufile then
			--- @TODO Check ramdisk modified here before to
			--- commando it !!
			deltree(ram_path.."/dcplaya") -- $$$ !! Outch !!
			result = vmu_load_file(vmufile,ram_path.."/dcplaya")
		     else
			result = vmu_init()
		     end
		  end,

      vmu_save  = function (menu)
		     vmu_save(nil,1)
		  end,

      vmu_saveas  = function (menu)
		       vmu_save_as()
		    end,

      vmu_autosave = function (menu, idx)
			local cc = menu.root_menu.target
			cc.vmu_auto_save = not cc.vmu_auto_save
			menu_yesno_image(menu, idx, cc.vmu_auto_save,
					 'auto-save')
		    end,

      vmu_deffile = function (menu, idx)
			vmu_no_default_file = not vmu_no_default_file
			menu_yesno_image(menu, idx, not vmu_no_default_file,
				       'use default')
		     end,

      vmu_confwrite = function (menu, idx)
			 vmu_never_confirm_write = not vmu_never_confirm_write
			 menu_yesno_image(menu, idx, not vmu_never_confirm_write,
					'confirm')
		      end,
      kbd_rule = function (menu, idx)
		    local i,v
		    local tbl = {"never","normal","nokbd"}
		    ke_set_active_rule(tbl[idx])
		    tbl[3] = "no keyboard"
		    for i,v in tbl do
		       menu_yesno_image(menu, i, i == idx, v, 1)
		    end
		    menu:draw()
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


   local krule = ke_set_active_rule()

   local def = {
      root=root,
      cb = cb,
      sub = {
	 vmu = {
	    root = ':vmu:visual >vmu_visual,options >vmu_option,load{vmu_load},merge{vmu_merge},save{vmu_save},save as{vmu_saveas}',
	    sub = {
	       vmu_visual = ':visual:none{setvmuvis},scope{setvmuvis},fft{setvmuvis},band{setvmuvis}',
	       vmu_option = ':option:'
		  .. menu_yesno_menu(cc.vmu_auto_save,
				     'auto-save','vmu_autosave') .. ','
		  .. menu_yesno_menu(not vmu_no_default_file,
				     'use default','vmu_deffile') .. ','
		  .. menu_yesno_menu(not vmu_never_confirm_write,
				     'confirm','vmu_confwrite'),
	    },
	 },
	 plugins = plugins,
	 keyboard = ':keyboard:'
	    .. menu_yesno_menu(krule == "never",'never','kbd_rule') .. ','
	    .. menu_yesno_menu(krule == "normal",'normal','kbd_rule') .. ','
	    .. menu_yesno_menu(krule == "nokbd",'no keyboard','kbd_rule'),
      }
   }
   return menu_create_defs(def , target)
end

--
--- control-center event handler.
---
--- @param  app  control center application
--- @param  evt  recieved event
---
--- @return event
---
--- @see dcplaya_lua_event
--- @internal
--
function control_center_handle(app, evt)
   local key = evt.key
   if key == evt_shutdown_event then
      dl_set_active(app.dl,nil)
      return evt
   end

   if ioctrl_ramdisk_event and key == ioctrl_ramdisk_event then
      if __DEBUG_EVT then
	 print("[control-center] : ioctrl_ramdisk_event");
      end
      if app.vmu_auto_save then
	 vmu_save(nil, 1)
      end
      return nil
   end
   
   if __DEBUG_EVT then
      printf("[cc] leave %d", key)
   end
   return evt
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
      handle = control_center_handle,
      update = nil,
      icon_name = "control-center",
      
      -- Members
      z = gui_guess_z(owner,z),
      fade = 0,
      dl = dl_new_list(256, 1),
      mainmenu_def = control_center_menucreator,
      flags = { unfocusable = 1 },

      vmu_auto_save = vmu_auto_save,
   }
   control_center_create_sprites(cc)

   evt_app_insert_last(owner, cc)
   return cc
end

--
--- Kill a control center application.
---
---   The control_center_kill() function kill the given application by
---   calling sending the evt_shutdown_app() function. If the given
---   application is nil or control_center the default control center
---   (control_center) is killed and the global control_center is set
---   to nil.
---
--- @param  cc  application to kill (default to control_center)
--
function control_center_kill(cc)
   cc = cc or control_center
   if type(cc) == "table" then
      evt_shutdown_app(cc)
      if cc == control_center then
	 control_center = nil
	 print("control-center shutdowed")
      end
   end
end


control_center_kill()
control_center = control_center_create()

if control_center then
   print("control-center running")
end

-- Load application icon
for k,v in { "dcplaya", "vmu32", "volume", "control-center", "keyboard" } do
   if not (tex_exist(v) or
	   tex_new(home .. "lua/rsc/icons/" .. v .. ".tga")) then
      print("[control-center] : no "..v.." icon found")
   end
end

--
--- @}
---

control_center_loaded = 1
return control_center_loaded
