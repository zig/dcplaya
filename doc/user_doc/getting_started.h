/*  dcplaya user documentation
 *  $Id: getting_started.h,v 1.1 2003-04-19 23:54:11 benjihan Exp $
 */

/**
@defgroup  dcplaya_getting_started  Getting started
@ingroup   dcplaya_user
@brief     getting started with dcplaya

dcplaya is more than a simple music player. It is a full development enviroment
and nearly an operating system. Simple dcplaya users do not need to understand
everything on dcplaya operating system. 

@par First help

Basically it is possible to control dcplaya with either any controller (connected in any port) or a keyboard controller.

@par Controlling dcplaya

- The controller <IMG src="buttonA.gif" alt="(A)"> button or ENTER key on the
  keyboard is called @b confirm because you should use it to confirm any
  action. It works like a mouse button left click on your favorite window
  manager.
- The controller <IMG src="buttonB.gif" alt="(B)"> button or ESC key on the
  keyboard is called @b cancel. The cancel action has many behaviours
  depending on the application.
  In most case it allow you quit a dialog without doing anything.
  In the song-browser application the cancel action depends on the
  focused window (either filelist or playlist). If the filelist window
  is focused cancel action will stop the current music from playing.
  If the playlist is focused the cancel action will delete the cursor
  entry without any warning.
- The controller <IMG src="buttonX.gif" alt="(X)"> button or BACKSPACE key
  on the keyboard is called @b menu. The menu action opens the contextual
  menu. Contextual means that the content of the menu depends on the current
  selected item. 
  It works like a mouse button right click on your favorite window manager.
  When the focused application is the desktop application, the menu action
  opens the selected application main menu.
- The controller <IMG src="buttonY.gif" alt="(Y)"> button or PRINT-SCREEN key
  on the keyboard is called ??? (well, it does not have official name,
  I just call it @b toggle here). The toggle action toggles the desktop
  application. This means that toggle action will open or close the desktop
  application if respectively it is currently close or open. The desktop
  application is one og the top most important application of dcplaya
  window manager. It allows you to choose the
  @ref focused_application_concept "focused application".
  and that is where you have access to the
  @ref main_menu_concept "application's main menu".

@author    benjamin gerard

 **/


