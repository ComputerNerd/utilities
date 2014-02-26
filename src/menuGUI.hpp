#ifndef __MENUGUI_H
#define __MENUGUI_H

#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <fxcg/rtc.h>
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MENUITEM_NORMAL 0
#define MENUITEM_CHECKBOX 1
#define MENUITEM_SEPARATOR 2
#define MENUITEM_VALUE_NONE 0
#define MENUITEM_VALUE_CHECKED 1
typedef struct
{
  char text[42]; // text to be shown on screen. This shouldn't take more than 21 chars (20 in case of checkboxes) but I'm giving 42 because of multibyte codes...
  //char tag[30]; // internal var of the menu item, may be useful for some thing. NOTE: commented because it is yet to be needed, and so it was wasting memory.
  //void (*handler)(void); // routine to call when user performs an action on this menu item. for normal menuitems, this is when it is selected (press EXE). for checkboxes, this is when the checkbox is toggled.
  short type=MENUITEM_NORMAL; // type of the menu item. use MENUITEM_* to set this
  short value=MENUITEM_VALUE_NONE; // value of the menu item. For example, if type is MENUITEM_CHECKBOX and the checkbox is checked, the value of this var will be MENUITEM_VALUE_CHECKED
  short color=TEXT_COLOR_BLACK; // color of the menu item (use TEXT_COLOR_* to define)
  // The following two settings require the menu type to be set to MENUTYPE_MULTISELECT
  short isfolder=0; // for file browsers, this will signal the item is a folder
  short isselected=0; // for file browsers and other multi-select screens, this will show an arrow before the item
  short icon=-1; //for file browsers, to show a file icon. -1 shows no icon (default)
} MenuItem;

typedef struct
{
  unsigned short data[0x12*0x18];
} MenuItemIcon;

#define MENUTYPE_NORMAL 0
#define MENUTYPE_MULTISELECT 1
#define MENUTYPE_NO_KEY_HANDLING 2 //this type of menu doesn't handle any keys, only draws.
#define MENUTYPE_FKEYS 3 // returns GetKey value of a Fkey when one is pressed
typedef struct {
  char statusText[75]; // text to be shown on the status bar, may be empty
  short showtitle=0; // whether to show a title as the first line
  char title[42]; // title to be shown on the first line if showtitle is !=0
  char* subtitle;
  short showsubtitle=0;
  short titleColor=TEXT_COLOR_BLUE; //color of the title
  char nodatamsg[42]; // message to show when there are no menu items to display
  short startX=1; //X where to start drawing the menu. NOTE this is not absolute pixel coordinates but rather character coordinates
  short startY=1; //Y where to start drawing the menu. NOTE this is not absolute pixel coordinates but rather character coordinates
  short width=21; // NOTE this is not absolute pixel coordinates but rather character coordinates
  short height=8; // NOTE this is not absolute pixel coordinates but rather character coordinates
  short scrollbar=1; // 1 to show scrollbar, 0 to not show it.
  short scrollout=0; // whether the scrollbar goes out of the menu area (1) or it overlaps some of the menu area (0)
  short numitems; // number of items in menu
  short type=MENUTYPE_NORMAL; // set to MENUTYPE_* .
  short selection=1; // currently selected item. starts counting at 1
  short scroll=0; // current scrolling position
  short fkeypage=0; // for MULTISELECT menu if it should allow file selecting and show the fkey label
  short numselitems=0; // number of selected items
  short returnOnInfiniteScrolling=0; //whether the menu should return when user reaches the last item and presses the down key (or the first item and presses the up key)
  short darken=0; // for dark theme on homeGUI menus
  short miniMiniTitle=0; // if true, title will be drawn in minimini. for calendar week view
  short useStatusText=0;
  short pBaRtR=0; //preserve Background And Return To Redraw. Rarely used
  MenuItem* items; // items in menu
} Menu;

#define MENU_RETURN_EXIT 0
#define MENU_RETURN_SELECTION 1
#define MENU_RETURN_INSTANT 2
#define MENU_RETURN_SCROLLING 3 //for returnOnInfiniteScrolling
#define MENU_RETURN_INTOSETTINGS 4 //basically this is to request a redraw when the user accesses the settings. Restoring the VRAM may not work when the user accesses the main menu while the settings menu is open
short doMenu(Menu* menu, MenuItemIcon* icontable=NULL);
short getMenuSelectionIgnoringSeparators(Menu* menu);
short getMenuSelectionOnlySeparators(Menu* menu);

void closeMsgBox();

#endif