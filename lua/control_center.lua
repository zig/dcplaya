--- @ingroup  dcplaya_lua_application
--- @file     control_center.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    control center application.
---
--- $Id: control_center.lua,v 1.1 2003-03-01 14:33:54 ben Exp $
---

control_center_loaded = nil

if not dolib("gui") then return end
if not dolib("menu") then return end

function control_center_create(owner, name)
   owner = owner or evt_desktop_app
   name = name or "control center"

   function control_center_volume(menu)
      local cc = menu.target
      print("cc-volume");
   end

   function control_center_about(menu)
      local cc = menu.target
      print("cc-about");
   end

   function control_center_menucreator(target)
      local root = ":" .. target.name .. ":" .. 
	 "about{about},volume{volume},vmu >vmu,plugins >plugins"

      local cb = {
	 about = control_center_about,
	 volume = control_center_volume,
	 setvmuvis = function (menu) 
			local cc = menu.root_menu.target
			local pos = menu.fl:get_pos()
			if pos then pos = pos - 1 end
			vmu_set_visual(pos)
		     end
      }
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
	    plugins = ":plugins:",
	 }
      }
      return menu_create_defs(def , target)
   end

   local cc = {
      -- Application
      name = name,
      version = 1.0,
      handle = nil,
      update = nil,
      
      -- Members
      z = gui_guess_z(owner,z),
      fade = 0,
      dl = dl_new_list(256, 1),
      mainmenu_def = control_center_menucreator
   }

   evt_app_insert_first(owner, cc)
   return cc
end

if control_center then
   evt_shutdown_app(control_center)
end

control_center = control_center_create()

function control_center_kill(cc)
   cc = cc or control_center
   if cc then
      evt_shutdown_app(cc)
      if cc == control_center then control_center = nil end
   end
end

control_center_loaded = 1
return control_center_loaded
