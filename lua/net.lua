
net_cfg_filename = "/ram/dcplaya/configs/net.cfg"

dofile(net_cfg_filename)

function net_save_settings()
   if gui_yesno("Do you want to save your settings on the VMU ?",
		nil, "Network settings", "Save", "Cancel") == 1 then

      if net_cfg then
	 local file = openfile(net_cfg_filename, "w")
	 if file then
	    write(file, "net_cfg = \n"..type_dump(net_cfg))
	    closefile(file)
	 end
      else
	 rm(net_cfg_filename)
      end
      if vmu_save then
	 vmu_save(nil, 1)
      end
   end
end

function net_connect_dialog()
   local answer = gui_yesno([[
Do you want to connect on a network (you need a BBA or a 
Lan adaptor for that)]],
nil, "Network settings", "Connect", "No network")

   if answer ~= 1 then
      net_cfg = nil
      net_save_settings()
      if net_disconnect then
	 net_disconnect()
      end
      return
   end

   answer = gui_ask([[
Choose how you want to connect.]],
{ "DHCP", "Manual settings" },
600, "Network settings")

   if answer == 1 then
      gui_ask("Sorry, this is not implemented yet ...", {"OK"})
   elseif answer == 2 then

      local text = '<dialog guiref="dialog" label="Network settings" name="Network settings">'

      text = text..'<vspace h="24">'
      text = text..'<button x="left" y="upout" create="gui_new_input" label="Ip :" name="192.168.1.9" total_w="300" guiref="ip"> <hspace w="1"> </button> <br>'
      text = text..'<vspace h="24">'
      text = text..'<button x="left" y="upout" create="gui_new_input" label="Mask :" name="255.255.255.0" total_w="300" guiref="mask"> <hspace w="1"> </button> <br>'
      text = text..'<vspace h="24">'
      text = text..'<button x="left" y="upout" create="gui_new_input" label="Gateway :" name="192.168.1.2" total_w="300" guiref="gw"> <hspace w="1"> </button> <br>'
      text = text..'<vspace h="24">'
      text = text..'<button x="left" y="upout" create="gui_new_input" label="DNS :" name="212.198.2.51" total_w="300" guiref="dns"> <hspace w="1"> </button> <br>'

      text = text..[[
<vspace h="16">
<center>
<button guiref="ok">OK</button>
<hspace w="16">
<button guiref="cancel">Cancel</button>
]]

      text = text.."</dialog>"

      local tt = tt_build(text, {x="center", y="center"})

      tt_draw(tt)

      local dialog = tt.guis.dialog

      local connect_ok = 
	 function()
	    evt_shutdown_app(%dialog)
	    %dialog.answer = 1
	 end

      local connect_cancel = 
	 function()
	    evt_shutdown_app(%dialog)
	    %dialog.answer = 2
	 end

      dialog.guis.ok.event_table[gui_press_event] = connect_ok
      dialog.guis.cancel.event_table[gui_press_event] = connect_cancel
      dialog.event_table[evt_shutdown_event] =
	 function(app) app.answer = 2 end

      while not dialog.answer do
	 evt_peek()
      end

      if dialog.answer == 1 then

	 net_cfg = {
	    ip = dialog.guis.ip.input,
	    mask = dialog.guis.mask.input,
	    gw = dialog.guis.gw.input,
	    dns = dialog.guis.dns.input,
	 }

	 net_autoconnect()

	 net_save_settings()
      end
      
   end
end


function net_autoconnect()
   if not net_cfg or not net_cfg.ip then
      return
   end

   if not net_connect then
      dl(plug_net)
   end

   if not net_connect then
      gui_ask("Could not load net driver !", {"OK"})
      return
   end


   sc()
   local scroll = gui_scrolltext and 
      gui_scrolltext(nil, "Bringing network up ...")

   local res = net_connect(net_cfg.ip)
   local text = "<left>"
   if not res then
      text = text..'net_connect ... OK <br>'
      res = net_lwip_init(net_cfg.ip, net_cfg.mask, net_cfg.gw)
      if not res then
	 text = text..'lwip_init ... OK '
	 text = text..[[
<vspace h="16">You should be able to use the network from now on !
<vspace h="16">
To access files on your PC, you need dcload-ip, then launch it twice like this : 
<font id="1" size="16" color="text_color"> <left>
<vspace h="16">
dc-tool -r <br>
dc-tool -z
<vspace h="16">
<font id="0" size="16"> <left>
On windoz, you can access only one of your drive, launch dc-tool from the drive
you want to access.
]]
	 net_resolv(net_cfg.dns)
      else
	 text = text..'lwip_init ... FAILED'
      end
   else
      text = text..'net_connect ... FAILED<br>Either you don\'t have any BBA or Lan adaptor, either you are already connected ?'
   end

   text = text.."<br><center>"

   hc()
   if scroll then dl_set_active(scroll, nil) end

   gui_ask(text, {"OK"})
end

--net_connect_dialog()

net_autoconnect()

return 1
