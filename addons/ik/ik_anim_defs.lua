--- @ingroup dcplaya_addons_ik
--- @file    ik_anim_defs.lua
--- @date    2002/12/19
--- @author  benjamin gerard <ben@sashipa.com>
--- @brief   Internationnal Karate Remix (animation definitions)
--

return {
   ["walk"] = {
      { "kamae0", 1 },
      { "kamae1", 1 },
      { "kamae2", 1 },
      { loop=-4 }
   },

   ["mpunch"] = {
      { "kamae0", 1 },
      { "mpunch0", 1.05 },
      { "mpunch1", 0.2 },
      { "mpunch1", 0 },
      { loop=-1 }
   },

   ["hpunch"] = {
      { "kamae0", 1 },
      { "hpunch0", 1 },
      { "hpunch1", 0.2 },
      { "hpunch1", 0 },
      { loop=-1 }
   },

   ["hkick"] = {
      { "kamae0", 1 },
      { "kick0", 1 },
      { "kick1", 1 },
      { "hkick3", 0.3 },
      { "hkick3", 0 },
      { loop=-1, speed=1.06 },
   },

   ["mkick"] = {
      { "kamae0", 1 },
      { "kick0", 1 },
      { "kick1", 1 },
      { "mkick3", 0.3 },
      { "mkick3", 0 },
      { loop=-1, speed=1.04 },
   },

   ["lkick"] = {
      { "kamae0", 1 },
      { "kick0", 1 },
      { "kick1", 1 },
      { "lkick3", 0.3 },
      { "lkick3", 0 },
      { "kick0", 1 },
      { "kamae0", 1 },
      { speed=1.02 },
   },

   ["backkick"] = {
      { "kamae0", 1 },
      { "back0", 1 },
      { "back1", 1 },
      { "back2", 1 },
      { "back3", 1 },
      { "back4", 1 },
      { "kick1", 1 },
      { "kick0", 1 },
      { "kamae0", 1 },
   },

   ["balayage"] = {
      { "kamae0", 1 },
      { "balayage0", 1 },
      { "balayage1", 1 },
      { "balayage0", 1 },
      { "kamae0", 1 },
   },

   ["dancing"] = {
      { "dancing0", 1 },
      { "dancing1", 1 },
      { "dancing2", 1 },
      { "dancing3", 1 },
      { loop=-3, speed=2, loop_start = 2 },
   },

   ["jump"] = {
      { "kamae0", 1 },
      { "jump0", 1 },
      { "jump1", 1 },
      { "jump2", 1 },
      { "jump2", 1 },
      { "jump1", 1 },
      { "kamae0", 1 },
   },

   ["coconut"] = {
      { "coconut0", 3 },
      { "coconut1", 1 },
      { "coconut2", 1 },
      { "coconut3", 4 },
      { speed=2 },
   },

   ["salut"] = {
      { "kamae0", 2 },
      { "salut0", 2 },
      { "salut1", 1.5 },
      { "salut2", 2 },
      { loop=-1, speed=1 },
   },

   ["koback"] = {
      { "koback0", 1 },
      { "koback1", 1 },
      { "koback2", 1 },
      { "koback3", 1 },
      { "koback4", 10 },
   },

}
