-- converted from kos keyboard.h

-- modifier keys
 KBD_MOD_LCTRL		=	(2^0)
 KBD_MOD_LSHIFT		=	(2^1)
 KBD_MOD_LALT		=	(2^2)
 KBD_MOD_S1		=	(2^3)
 KBD_MOD_RCTRL		=	(2^4)
 KBD_MOD_RSHIFT		=	(2^5)
 KBD_MOD_RALT		=	(2^6)
 KBD_MOD_S2		=	(2^7)

-- bits for leds : this is not comprensive (need for japanese kbds also)
 KBD_LED_NUMLOCK	=	(2^0)
 KBD_LED_CAPSLOCK	=	(2^1)
 KBD_LED_SCRLOCK	=	(2^2)


function keycode(s)
	local v = 0
	local i
	local l = strlen(s)
	
	for i=1, l, 1 do
		local c
		c = strbyte(s, i)
		if c >= 97 then
			c = c - 97 + 10
		elseif c >= 65 then
			c = c - 65 + 10
		else
			c = c - 48
		end
		v = v * 16 + c
	end

	return v*256
end


-- defines for the keys (argh...)
-- these are raw key codes
 KBD_KEY_NONE	=		keycode[[00]]
 KBD_KEY_ERROR	=		keycode[[01]]
 KBD_KEY_A	=		keycode[[04]]
 KBD_KEY_B	=		keycode[[05]]
 KBD_KEY_C	=		keycode[[06]]
 KBD_KEY_D	=		keycode[[07]]
 KBD_KEY_E	=		keycode[[08]]
 KBD_KEY_F	=		keycode[[09]]
 KBD_KEY_G	=		keycode[[0a]]
 KBD_KEY_H	=		keycode[[0b]]
 KBD_KEY_I	=		keycode[[0c]]
 KBD_KEY_J	=		keycode[[0d]]
 KBD_KEY_K	=		keycode[[0e]]
 KBD_KEY_L	=		keycode[[0f]]
 KBD_KEY_M	=		keycode[[10]]
 KBD_KEY_N	=		keycode[[11]]
 KBD_KEY_O	=		keycode[[12]]
 KBD_KEY_P	=		keycode[[13]]
 KBD_KEY_Q	=		keycode[[14]]
 KBD_KEY_R	=		keycode[[15]]
 KBD_KEY_S	=		keycode[[16]]
 KBD_KEY_T	=		keycode[[17]]
 KBD_KEY_U	=		keycode[[18]]
 KBD_KEY_V	=		keycode[[19]]
 KBD_KEY_W	=		keycode[[1a]]
 KBD_KEY_X	=		keycode[[1b]]
 KBD_KEY_Y	=		keycode[[1c]]
 KBD_KEY_Z	=		keycode[[1d]]
 KBD_KEY_1	=		keycode[[1e]]
 KBD_KEY_2	=		keycode[[1f]]
 KBD_KEY_3	=		keycode[[20]]
 KBD_KEY_4	=		keycode[[21]]
 KBD_KEY_5	=		keycode[[22]]
 KBD_KEY_6	=		keycode[[23]]
 KBD_KEY_7	=		keycode[[24]]
 KBD_KEY_8	=		keycode[[25]]
 KBD_KEY_9	=		keycode[[26]]
 KBD_KEY_0	=		keycode[[27]]
 KBD_KEY_ENTER	=		keycode[[28]]
 KBD_KEY_ESCAPE	=		keycode[[29]]
 KBD_KEY_BACKSPACE	=	keycode[[2a]]
 KBD_KEY_TAB	=		keycode[[2b]]
 KBD_KEY_SPACE	=		keycode[[2c]]
 KBD_KEY_MINUS	=		keycode[[2d]]
 KBD_KEY_PLUS	=		keycode[[2e]]
 KBD_KEY_LBRACKET	=	keycode[[2f]]
 KBD_KEY_RBRACKET	=	keycode[[30]]
 KBD_KEY_BACKSLASH	=	keycode[[31]]
 KBD_KEY_SEMICOLON	=	keycode[[33]]
 KBD_KEY_QUOTE	=		keycode[[34]]
 KBD_KEY_TILDE	=		keycode[[35]]
 KBD_KEY_COMMA	=		keycode[[36]]
 KBD_KEY_PERIOD	=		keycode[[37]]
 KBD_KEY_SLASH	=		keycode[[38]]
 KBD_KEY_CAPSLOCK	=	keycode[[39]]
 KBD_KEY_F1	=		keycode[[3a]]
 KBD_KEY_F2	=		keycode[[3b]]
 KBD_KEY_F3	=		keycode[[3c]]
 KBD_KEY_F4	=		keycode[[3d]]
 KBD_KEY_F5	=		keycode[[3e]]
 KBD_KEY_F6	=		keycode[[3f]]
 KBD_KEY_F7	=		keycode[[40]]
 KBD_KEY_F8	=		keycode[[41]]
 KBD_KEY_F9	=		keycode[[42]]
 KBD_KEY_F10	=		keycode[[43]]
 KBD_KEY_F11	=		keycode[[44]]
 KBD_KEY_F12	=		keycode[[45]]
 KBD_KEY_PRINT	=		keycode[[46]]
 KBD_KEY_SCRLOCK	=	keycode[[47]]
 KBD_KEY_PAUSE	=		keycode[[48]]
 KBD_KEY_INSERT	=		keycode[[49]]
 KBD_KEY_HOME	=		keycode[[4a]]
 KBD_KEY_PGUP	=		keycode[[4b]]
 KBD_KEY_DEL	=		keycode[[4c]]
 KBD_KEY_END	=		keycode[[4d]]
 KBD_KEY_PGDOWN	=		keycode[[4e]]
 KBD_KEY_RIGHT	=		keycode[[4f]]
 KBD_KEY_LEFT	=		keycode[[50]]
 KBD_KEY_DOWN	=		keycode[[51]]
 KBD_KEY_UP	=		keycode[[52]]
 KBD_KEY_PAD_NUMLOCK	=	keycode[[53]]
 KBD_KEY_PAD_DIVIDE	=	keycode[[54]]
 KBD_KEY_PAD_MULTIPLY	=	keycode[[55]]
 KBD_KEY_PAD_MINUS	=	keycode[[56]]
 KBD_KEY_PAD_PLUS	=	keycode[[57]]
 KBD_KEY_PAD_ENTER	=	keycode[[58]]
 KBD_KEY_PAD_1		=	keycode[[59]]
 KBD_KEY_PAD_2		=	keycode[[5a]]
 KBD_KEY_PAD_3		=	keycode[[5b]]
 KBD_KEY_PAD_4		=	keycode[[5c]]
 KBD_KEY_PAD_5		=	keycode[[5d]]
 KBD_KEY_PAD_6		=	keycode[[5e]]
 KBD_KEY_PAD_7		=	keycode[[5f]]
 KBD_KEY_PAD_8		=	keycode[[60]]
 KBD_KEY_PAD_9		=	keycode[[61]]
 KBD_KEY_PAD_0		=	keycode[[62]]
 KBD_KEY_PAD_PERIOD	=	keycode[[63]]
 KBD_KEY_S3		=	keycode[[65]]



-- standard keys
 KBD_BACKSPACE	=	8
 KBD_ENTER	=	13


