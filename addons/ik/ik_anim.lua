--- @ingroup dcplaya_addons_ik
--- @file    ik_amin.lua
--- @date    2002/12/19
--- @author  benjamin gerard <ben@sashipa.com>
--- @brief   Internationnal Karate Remix (animation player)
--

--
--- struct anim {
--
---   table ref;         /**< Reference animation. */
---   number speed;      /**< Play speed factor.   */
---   boolean pingpong;  /**< */
---   number loop;        /**< Number of loop or nil */
---   number cur_loop;   /**< Current loop */
---   number cur_key;    /**< Current key index. */
---   number prev_key;   /**< Previous key index. */
---   number cur_time;   /**< Current playing time. */

---   number cur_start;  /**< Current anim start point. */
---   number cur_stop    /**< Current anim stop point.  */

--
--- };
--

--- Start a animation
---
--- @return struct anim
--
function ik_anim_start(anim_ref, speed, loop, time)
   -- Set default parameters
   time = time or 0
   speed = (speed or 1) * (anim_ref.speed or 1)
   loop = loop or anim_ref.loop

   local anim = {
      ref = anim_ref,
      speed = speed,
   }
   
   if loop then
      anim.loop = abs(loop)
      anim.pingpong = loop < 0
   end

   ik_anim_set_time(anim, time)
   return anim
end

--
--- Set anim to a given time position
--- @param anim
--- @param time
--- @return anim or nil
--
function ik_anim_set_time(anim, time)
   anim.cur_key = 1
   anim.prev_key = nil
   anim.cur_time = 0
   anim.cur_key_time = 0
   anim.cur_loop = 1

   anim.step = 1
   anim.cur_start = 1
   anim.cur_stop = getn(anim.ref)

   anim.loop_start = anim.ref.loop_start or anim.cur_start
   anim.loop_stop  = anim.ref.loop_stop or anim.cur_stop
   if anim.loop_stop < anim.loop_start then
      anim.loop_start, anim.loop_stop = anim.loop_stop, anim.loop_start
   end

   return ik_anim_play(anim,time)
end

function ik_anim_play(anim, elapsed_time)
   local time, key_time = anim.cur_time+elapsed_time, anim.cur_key_time
   local speed, step = anim.speed, anim.step
   local start, stop = anim.cur_start, anim.cur_stop
   local cur, prev = anim.cur_key, anim.prev_key

   repeat
      local key = anim.ref[cur]
      local time_end = key_time + key.time * speed

      anim.wait = (key_time == time_end) and not anim.release
      anim.release = nil

      -- Test release position or inside the key frame time range
      if anim.wait or (time >= key_time and time < time_end) then
	 -- Found it !
	 key_time = (anim.wait and time) or key_time
	 anim.cur_start, anim.cur_stop = start, stop
	 anim.cur_key, anim.prev_key = cur, prev
	 anim.cur_time, anim.cur_key_time = time, key_time
	 anim.step = step
	 return
      end

      -- Change key frame
      key_time = time_end
      prev = cur
      cur = cur + step

      -- Check for loop
      if cur < start or cur > stop then
	 -- Key out of anim
	 if not anim.loop  then
	    -- No loop : finish anim on stop frame
	    cur = stop
	    step = 0
	 elseif not anim.pingpong then
	    -- Loop at start
	    if anim.cur_loop >= anim.loop then
	       -- Finish on stop frame
	       cur = stop
	       step = 0
	    else
	       anim.cur_loop = anim.cur_loop + 1
	       cur = anim.loop_start
	       step = 1
	    end
	 else 
	    -- Ping pong looping
	    if cur > stop then
	       -- loop back, do not increment loop counter
	       cur = stop - 1
	       step = -1
	    else
	       if anim.cur_loop >= anim.loop then
		  -- Finish on start frame
		  cur = start
		  step = 0
	       else
		  anim.cur_loop = anim.cur_loop + 1
		  cur = start+1
		  step = 1
	       end
	    end
	 end
	 start, stop = anim.loop_start, anim.loop_stop
      end
   until nil
end

--
--- Draw anim at current time position.
--
--- @param  anim    Anim current state.
--- @param  dl      Display list.
--- @param  name    Draw sprite name if not nil.
--- @param  attack  Draw attack box if not nil.
--
function ik_anim_draw(anim, dl, name, attack, target)
   local ref = anim.ref[anim.cur_key]
   local spr1 = ref.sprite
   local spr2 = anim.nxt_key and anim.ref[anim.nxt_key].sprite

   -- Draw sprite
   spr1.sprite:draw(dl)

   -- Draw sprite name
   if name and spr1.name then
      dl_draw_text(dl, 0, 5, 2, 1,1,1,1, spr1.name)
   end

   local r = (anim.cur_time - anim.cur_key_time) / (ref.time * anim.speed)

   -- Draw attack box
   if attack and spr1.attack then
      ik_draw_colide_box(dl,spr1.attack,spr2 and spr2.attack,r,{0.7, 1,1,0})
   end

   -- Draw target boxes
   if target and spr1.targets then
      local i,v
      for i,v in spr1.targets do
	 ik_draw_colide_box(dl,v,nil,nil,{0.7, 1,0,0})
      end
   end
end

function ik_draw_colide_box(dl, ab, cd, r, col)
   if ab2 then
      ab = ab * (1-r) + ab2 * r
   end
   dl_draw_box1(dl, ab[1],ab[2], ab[1]+ab[3], ab[2]+ab[4], 10,
		col[1]*ab[5], col[2], col[3], col[4])
end

return 1
