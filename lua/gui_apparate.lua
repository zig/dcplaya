--
-- funky gui dialog box apparition
--
-- author : vincent penne
--


gui_apparate_list = { }


-- called by the desktop
function gui_apparate_update(app, ft)

   local app, a
   local remove

   for app, a in gui_apparate_list do

      if a.first == 1 then
	 a.first = 2
      else

	 local scale = a.scale
	 local dl = a.dl

	 if scale < 1.005 then
	    scale = 1
	 end
	 if scale > 9 then
	    scale = 10
	 end

	 scale = scale + 20 * ft * (a.aim-scale)
	 a.scale = scale

	 if a.first == 2 then
	    dl_set_active(dl, 1)
--	    if a.odl then
--	       dl_set_active(a.odl, 1)
--	    end
	    a.first = nil
	 end

	 if scale == 1 or scale == 10 then
	    if scale == 10 then
	       dl_set_active(dl, nil)
	    end
	    if not remove then
	       remove = { }
	    end
	    tinsert(remove, app)
	 end

	 local col = 1 - (scale-1)/8
	 scale = scale / 4 + 0.75
	 local scalex = scale
	 local scaley = 1/scale --scale
	 dl_set_trans(dl, 
		      mat_scale(scalex, scaley, 1)
			 * mat_trans(320 * (1-scalex), 240 * (1-scaley), 0)
		   )
	 dl_set_color(dl, col, 1, col, 1, 1)

      end
   end

   if remove then
      local i
      for i=1, getn(remove), 1 do
	 gui_apparate_list[remove[i]] = nil
      end
   end
   
end

-- call this to make your dialog box apparating
function gui_apparate(app, aim)
   
   local dl = app.dl or app.dl
   aim = aim or 1

   if not dl then
      print("gui_apparate : no display list in your dialog")
   end

--   local odl
--   if dl ~= app._dl then
--      odl = app._dl
--   end

   local a = gui_apparate_list[app]
   if a then
      a.aim = aim
   else
      a = {
	 scale = 11-aim,
	 dl = dl,
	 --odl = odl,
	 first = 1,
	 aim = aim,
      }
   end

   gui_apparate_list[app] = a

end

function gui_disaparate(app)
   gui_apparate(app, 10)
end

return 1
