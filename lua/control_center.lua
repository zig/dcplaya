--- @ingroup  dcplaya_lua_application
--- @file     control_center.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    control center application.
---
--- $Id: control_center.lua,v 1.4 2003-03-03 08:35:24 ben Exp $
---

control_center_loaded = nil

if not dolib("gui") then return end
if not dolib("menu") then return end
if not dolib("volume_control") then return end

function control_center_create(owner, name)
   owner = owner or evt_desktop_app
   name = name or "control center"

   function control_center_volume(menu)
      local cc = menu.target
      print("cc-volume II");
      volume_control_create(); -- $$$ do not works with cc has owner
   end

   function control_center_about(menu)
      local cc = menu.target

      print("cc-about");
   end


   function control_center_create_sprite(cc, name, src,
					 w, h,
					 u1, v2, u2, v2,
					 rotate)
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

   function control_center_create_sprites(cc)
      control_center_create_sprite(cc, "cc_dcp", "dcplaya.tga", 32)
      control_center_create_sprite(cc, "cc_vmu", "vmu32.tga",32)
      control_center_create_sprite(cc, "cc_vol", "volumectrl.tga", 32)
--				   24, 24) --, 0 , 0, 8/64, 48/64)
   end

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

   evt_app_insert_last(owner, cc)
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
