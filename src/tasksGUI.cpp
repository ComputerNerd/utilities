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
#include <alloca.h>

#include "tasksGUI.hpp"
#include "menuGUI.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "calendarProvider.hpp"
#include "calendarGUI.hpp"
#include "constantsProvider.hpp"

void viewTasks() {
  Menu menu;
  
  menu.scrollout=1;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  menu.nodatamsg = (char*)"No tasks - press F2";
  menu.title = (char*)"Tasks";
  menu.returnOnRight = 1;
  while(viewTasksChild(&menu));
}

int viewTasksChild(Menu* menu) {
  //returns 1 when it wants to be restarted (refresh tasks)
  //returns 0 if the idea really is to exit the screen
  EventDate taskday;
  
  menu->numitems = getEvents(&taskday, CALENDARFOLDER, NULL); // get event count
  CalendarEvent* tasks = (CalendarEvent*)alloca(menu->numitems*sizeof(CalendarEvent));
  MenuItem* menuitems = (MenuItem*)alloca(menu->numitems*sizeof(MenuItem));
  menu->numitems = getEvents(&taskday, CALENDARFOLDER, tasks);
  int activecount = 0;
  for(int curitem = 0; curitem < menu->numitems; curitem++) {
    menuitems[curitem].text = (char*)tasks[curitem].title;
    menuitems[curitem].type = MENUITEM_CHECKBOX;
    menuitems[curitem].value = tasks[curitem].repeat;
    if(!menuitems[curitem].value) activecount++;
    menuitems[curitem].color = tasks[curitem].category-1;
  }
  menu->items=menuitems;
  
  char subtitle[50];
  sprintf(subtitle, "%d tasks, %d active", menu->numitems, activecount);
  menu->subtitle = subtitle;
  while(1) {
    Bdisp_AllClr_VRAM();
    // VIEW, INSERT, EDIT, DELETE, DEL-ALL, Switch [white]
    drawFkeyLabels(-1, 0x03B4);
    if(menu->numitems>0) drawFkeyLabels(0x049F, -1, 0x0185, 0x0038, 0x0104, 0x049D);
    int res = doMenu(menu);
    switch(res) {
      case MENU_RETURN_EXIT:
        return 0;
        break;
      case KEY_CTRL_F1:
      case MENU_RETURN_SELECTION:
        if(menu->numitems>0) viewEvent(&tasks[menu->selection-1]);
        break;
      case KEY_CTRL_F2:
        if(menu->numitems >= MAX_DAY_EVENTS) {
          AUX_DisplayErrorMessage( 0x2E );
        } else {
          if(EVENTEDITOR_RETURN_CONFIRM == eventEditor(0, 0, 0, EVENTEDITORTYPE_ADD, NULL, 1)) {
            return 1;
          }
        }
        break;
      case KEY_CTRL_F3:
        if(menu->numitems > 0) {
          if(eventEditor(0, 0, 0, EVENTEDITORTYPE_EDIT, &tasks[menu->selection-1], 1) ==
             EVENTEDITOR_RETURN_CONFIRM)
            replaceEventFile(&tasks[menu->selection-1].startdate, tasks, CALENDARFOLDER,
                             menu->numitems);
          return 1;
        }
        break;
      case KEY_CTRL_F4:
      case KEY_CTRL_DEL:
        if(menu->numitems > 0)
          if(EVENTDELETE_RETURN_CONFIRM ==
             deleteEventPrompt(tasks, menu->numitems, menu->selection-1, 1))
            return 1;
        break;
      case KEY_CTRL_F5:
        if(menu->numitems > 0)
          if(EVENTDELETE_RETURN_CONFIRM == deleteAllEventsPrompt(0, 0, 0, 1))
            return 1;
        break;
      case KEY_CTRL_F6:
        if(menu->numitems > 0) {
          tasks[menu->selection-1].repeat = !tasks[menu->selection-1].repeat;
          replaceEventFile(&tasks[menu->selection-1].startdate, tasks, CALENDARFOLDER,
                           menu->numitems);
          return 1;
        }
        break;
      case KEY_CTRL_FORMAT:
        if(menu->numitems > 0) {
          //the "FORMAT" key is used in many places in the OS to format e.g. the color of a field,
          //so in this add-in it is used to change the category (color) of a task/calendar event.
          if(EVENTEDITOR_RETURN_CONFIRM == setCategoryPrompt(&tasks[menu->selection-1])) {
            replaceEventFile(&tasks[menu->selection-1].startdate, tasks, CALENDARFOLDER,
                             menu->numitems);
            return 1;
          }
        }
        break;
    }
  }
  return 1;
}