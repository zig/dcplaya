--- @ingroup  dcplaya_lua_application
--- @file     control_center.lua
--- @author   benjamin gerard <ben@sashipa.com>
--- @date     2002
--- @brief    control center application.
---
--- $Id: control_center.lua,v 1.5 2003-03-03 11:32:32 ben Exp $
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

      local macro =
	 '<macro macro-name="tfont" macro-cmd="font" color="#FFE080" size="20"><macro macro-name="url" macro-cmd="font" color="#80E0FF" size="14" font_id="1"><macro macro-name="normal" macro-cmd="font" color="#e0e080" size="16"><macro macro-name="author" macro-cmd="font" color="#ff8080" size="18"><macro macro-name="pspace" macro-cmd="vspace" h="12">'

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
      control_center_create_sprite(cc, "cc_vol", "volume.tga", 32)
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

function control_center_kill(cc)
   cc = cc or control_center
   if cc then
      evt_shutdown_app(cc)
      if cc == control_center then control_center = nil end
   end
end

control_center_loaded = 1
return control_center_loaded
