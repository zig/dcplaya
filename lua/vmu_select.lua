
vmu_select_loaded = nil
if not dolib("textlist") then return end
if not dolib("gui") then return end
if not dolib("sprite") then return end

function vmu_select_create_sprites(vs)
   vs.sprites = {}
end

function vmu_select_create(owner, name, dir, x, y, z)

   -- Default
   owner = owner or evt_desktop_app
   name = name or "vmu select"

   -- VMU-select default style
   -- ------------------------
   local style = {
	  bkg_color		= { 0.8, 0.7, 0.7, 0.7,  0.8, 0.3, 0.3, 0.3 },
	  border		= 8,
	  span          = 1,
	  file_color	= { 1, 0, 0, 0 },
	  dir_color		= { 1, 0, 0, .4 },
	  cur_color		= { 1, 1, 1, 1,  1, 0.1, 0.4, 0.5 },
	  text          = {font=0, size=16, aspect=1}
   }

   --- VMU-select event handler.
   --
   function vmu_select_handle(dial,evt)
	  local key = evt.key

	  print ("EVT:"..key)
	  if key == gui_item_confirm_event then
		 print("CONFIRM")
		 return
	  elseif key == gui_item_cancel_event then
		 evt_shutdown_app(dial)
		 return
	  end
	  return evt
   end

   -- Create sprite
   local texid = tex_get("dcpsprites") or tex_new("/rd/dcpsprites.tga")
   local vmusprite = sprite("vmu",	
							0, 62/2,
							104, 62,
							108/512, 65/128, 212/512, 127/128,
							texid,1)

   local border = 8
   local w = vmusprite.w * 2 + 2 * (border + style.border)
   local h = vmusprite.h + 16 + 2 * (border + style.border) + 16
   local box = { 0, 0, w, h }
   local screenw, screenh = 640,480

   x = x or ((screenw - box[3])/2) 
   y = y or ((screenh - box[4])/2)
   local x2,y2 = x+box[3], y+box[4]
	
   -- Create dialog
   local dial
   dial = gui_new_dialog(owner,
						 {x, y, x2, y2 }, z, nil, name,
						 { x = "left", y = "up" } )
   dial.event_table = {
	  [gui_item_confirm_event]	= vmu_select_handle,
	  [gui_item_cancel_event]	= vmu_select_handle,
	  [gui_item_change_event]	= vmu_select_handle,
   }

   box = box + { x+border, y+16+border, x-border, y-border }
   local w,h = box[3]-box[1], box[4]-box[2]

   dial.vs = gui_textlist(dial,
		{
		   pos = {box[1], box[2]},
		   box = {w,h,w,h},
		   flags=nil,
		   filecolor = style.file_color,
		   dircolor  = style.dir_color,
		   bkgcolor  = style.bkg_color,
		   curcolor  = style.cur_color,
		   border    = style.border,
		   span      = style.span,
		})

   if not dial.vs then
	  print("vmu_select: error creating textlist-gui")
	  return
   end

   -- Customize textlist
   local fl = dial.vs.fl
   fl.vmusprite = vmusprite
   fl.measure_text = function(fl, entry)
						local w, h = dl_measure_text(fl.cdl,entry.name)
						return max(w,fl.vmusprite.w),
						h+fl.vmusprite.h+2*fl.span
					 end

   fl.draw_entry = function (fl, dl, entry, x , y, z)
					  local color = fl.dircolor
					  local wt,ht = dl_measure_text(dl,entry.name)
					  x = fl.bo2[1] * 0.5 - fl.border
					  local xt = x - wt * 0.5
					  dl_draw_text(dl,
								   xt, y, z+0.1,
								   color[1],color[2],color[3],color[4],
								   entry.name)
					  fl.vmusprite:draw(dl, x, y + ht, z)
				   end

   fl.draw_cursor = function () end
   
   fl:change_dir(dir or dirlist("-n","/vmu"))

   return dial
end

vs = vmu_select_create()
function k()
   if vs then evt_shutdown_app(vs) end
   vs = nil
end

-- getchar()
-- k()

-- vmu_select_loaded = 1
-- return vmu_select_loaded
