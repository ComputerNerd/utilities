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

#include "powerGUI.hpp"
#include "menuGUI.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "textGUI.hpp"
#include "settingsProvider.hpp"
#include "settingsGUI.hpp" 
#include "selectorGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "lightGUI.hpp"
#include "calendarGUI.hpp"
#include "inputGUI.hpp"

static int lastChronoComplete=0; // 1-based. 0 means no chrono completed. for notification on home screen.

inline void formatChronoString(chronometer* tchrono, int num, unsigned char* string)
{
  long long int unixtime = currentUnixTime();
  long long int unixdiff;
  char buffer[20];
  itoa(num, (unsigned char*)buffer);
  if(tchrono->state == CHRONO_STATE_CLEARED) {
    strcpy((char*)string, "\xe6\xa6");
  } else if (tchrono->state == CHRONO_STATE_RUNNING) {
    if(tchrono->type == CHRONO_TYPE_UP) strcpy((char*)string, "\xe6\x9C");
    else strcpy((char*)string, "\xe6\x9D"); 
  } else {
    if(tchrono->type == CHRONO_TYPE_UP) strcpy((char*)string, "\xe6\xAC");
    else strcpy((char*)string, "\xe6\xAD"); 
  }
  strcat((char*)string, buffer);
  strcat((char*)string, ":");
  
  if(tchrono->state == CHRONO_STATE_CLEARED) { return; } //nothing else to add, chrono is clear
  else if(tchrono->state == CHRONO_STATE_STOPPED) {
    //diff will be calculated in a different way, so that it is always stopped
    if(tchrono->type == CHRONO_TYPE_DOWN) unixdiff = tchrono->starttime+tchrono->duration-tchrono->laststop;
    else unixdiff = tchrono->laststop-tchrono->starttime;    
  } else {
    if(tchrono->type == CHRONO_TYPE_DOWN) unixdiff = tchrono->starttime+tchrono->duration-unixtime;
    else unixdiff = unixtime-tchrono->starttime;
  }

  long long int days,hours,minutes,seconds,milliseconds;

  milliseconds=unixdiff;  
  seconds = milliseconds / 1000;
  milliseconds %= 1000;
  minutes = seconds / 60;
  seconds %= 60;
  hours = minutes / 60;
  minutes %= 60;
  days = hours / 24;
  hours %= 24;

  if (days) {
    itoa(days, (unsigned char*)buffer);
    strcat((char*)string, buffer);
    strcat((char*)string, (char*)"\xe7\x64"); // small "d"
  }
  
  itoa(hours, (unsigned char*)buffer);
  if (hours < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(minutes, (unsigned char*)buffer);
  if (minutes < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  strcat((char*)string, ":");

  itoa(seconds, (unsigned char*)buffer);
  if (seconds < 10) strcat((char*)string, "0");
  strcat((char*)string, buffer);
  
  strcat((char*)string, ".");
  itoa(milliseconds, (unsigned char*)buffer);
  strcat((char*)string, buffer);
}
void doNothing() {}
static int stubTimer=0;
// for the mGetKey function to call before longjump, when user presses Shift+Exit
void stopAndUninstallStubTimer() {
  if(stubTimer>0) {
    Timer_Stop(stubTimer);
    Timer_Deinstall(stubTimer);
    stubTimer=0;
  }
}
void chronoScreen(chronometer* chrono) {
  lastChronoComplete = 0; // clear possible notification on home screen
  // setting a timer is needed to change some aspects of GetKeyWait_OS
  stubTimer = Timer_Install(0, doNothing, 50);
  if (stubTimer > 0) { Timer_Start(stubTimer); }
  
  // construct menu items
  MenuItem menuitems[NUMBER_OF_CHRONO];
  int curcolor = TEXT_COLOR_BLUE;
  for(int curitem=0; curitem < NUMBER_OF_CHRONO; curitem++) {
    menuitems[curitem].type = MENUITEM_CHECKBOX;
    menuitems[curitem].color = curcolor;
    switch(curcolor) {
      case TEXT_COLOR_BLUE: curcolor = TEXT_COLOR_RED; break;
      case TEXT_COLOR_RED: curcolor = TEXT_COLOR_GREEN; break;
      case TEXT_COLOR_GREEN: curcolor = TEXT_COLOR_PURPLE; break;
      case TEXT_COLOR_PURPLE: curcolor = TEXT_COLOR_BLACK; break;
      case TEXT_COLOR_BLACK: curcolor = TEXT_COLOR_BLUE; break;
    }
  }
  Menu menu;
  menu.items=menuitems;
  menu.numitems=NUMBER_OF_CHRONO;
  menu.type=MENUTYPE_NO_KEY_HANDLING; // NOTE doMenu won't handle keys for us!
  menu.height=7;
  menu.scrollout=1;
  menu.title = (char*)"Chronometers";
  
  short unsigned int key;
  while(1) {
    checkChronoComplete();
    unsigned char text[NUMBER_OF_CHRONO][42];
    int hasSelection = 0;
    for(int curitem=0; curitem < NUMBER_OF_CHRONO; curitem++) {
      formatChronoString(&chrono[curitem], curitem+1, text[curitem]);
      menuitems[curitem].text = (char*)text[curitem];
      if(menuitems[curitem].value) hasSelection = 1;
    }
    if(menu.fkeypage==0) {
      drawFkeyLabels(0x0037, // SELECT (white)
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED ? 0x0010 : 0), // SET
        (hasSelection || chrono[menu.selection-1].state != CHRONO_STATE_CLEARED ? 0x0149 : 0), // CLEAR
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_STOPPED ? 0x040A : 0), // play icon
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_RUNNING ? 0x0031 : 0), // stop icon
        (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED ? 0x0156 : 0)); // BUILT-IN
      // "hack" the stop icon, turning it into a pause icon
      drawRectangle(286, 197, 4, 14, COLOR_WHITE);
    } else if (menu.fkeypage==1) {
      // SELECT (white), ALL (white), None (white), REVERSE (white), empty, empty
      drawFkeyLabels(0x0037, 0x0398, 0x016A, 0x045B, 0, 0);
    }
    DisplayStatusArea();
    doMenu(&menu);
    
    int keyCol = 0, keyRow = 0;
    Bdisp_PutDisp_DD();
    GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key); // just to handle the Menu key
    key = PRGM_GetKey();
    switch(key)
    {
      case KEY_PRGM_SHIFT:
        //turn on/off shift manually because getkeywait doesn't do it
        SetSetupSetting( (unsigned int)0x14, (GetSetupSetting( (unsigned int)0x14) == 0));
        break;
      case KEY_PRGM_MENU:
        if (GetSetupSetting( (unsigned int)0x14) == 1) {
          SetSetupSetting( (unsigned int)0x14, 0);
          saveVRAMandCallSettings();
        }
        break;
      case KEY_PRGM_DOWN:
        if(menu.selection == menu.numitems) {
          menu.selection = 1;
          menu.scroll = 0;
        } else {
          menu.selection++;
          if(menu.selection > menu.scroll+(menu.numitems>6 ? 6 : menu.numitems))
            menu.scroll = menu.selection -(menu.numitems>6 ? 6 : menu.numitems);
        }
        break;
      case KEY_PRGM_UP:
        if(menu.selection == 1) {
          menu.selection = menu.numitems;
          menu.scroll = menu.selection-6;
        } else {
          menu.selection--;
          if(menu.selection-1 < menu.scroll)
            menu.scroll = menu.selection -1;
        }
        break;
      case KEY_PRGM_F1:
        if (GetSetupSetting( (unsigned int)0x14) == 1) {
          SetSetupSetting( (unsigned int)0x14, 0);
          menu.fkeypage=1;
        } else {
          if(menuitems[menu.selection-1].value == MENUITEM_VALUE_CHECKED) menuitems[menu.selection-1].value=MENUITEM_VALUE_NONE;
          else menuitems[menu.selection-1].value=MENUITEM_VALUE_CHECKED;
        }
        break;
      case KEY_PRGM_F2:
        if(menu.fkeypage==0 && (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED)) setChronoGUI(&menu, chrono);
        else if (menu.fkeypage==1) {
          // select all
          for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) menu.items[cur].value = MENUITEM_VALUE_CHECKED;
        }
        break;
      case KEY_PRGM_F3:
        if(menu.fkeypage==0) clearSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
        else if (menu.fkeypage==1) {
          // select none
          for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) menu.items[cur].value = MENUITEM_VALUE_NONE;
        }
        break;
      case KEY_PRGM_F4:
        if(menu.fkeypage==0) startSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
        else if (menu.fkeypage==1) {
          // reverse selection
          for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) menu.items[cur].value = !menu.items[cur].value;
        }
        break;
      case KEY_PRGM_F5:
        if(menu.fkeypage==0) stopSelectedChronos(&menu, chrono, NUMBER_OF_CHRONO);
        break;
      case KEY_PRGM_F6:
        if(menu.fkeypage==0 && (hasSelection || chrono[menu.selection-1].state == CHRONO_STATE_CLEARED)) setBuiltinChrono(&menu, chrono);
        break;
      case KEY_PRGM_EXIT:
        if(menu.fkeypage==0) {
          stopAndUninstallStubTimer();
          return;
        }
        else menu.fkeypage=0;
        break;
    }
    if (key && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
  }
}

void startSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int hasPerformedAny = 0;
  for(int cur = 0; cur < count; cur++) {
    if(menu->items[cur].value) {
      startChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
  }
  if(!hasPerformedAny) startChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}

void stopSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int hasPerformedAny = 0;
  for(int cur = 0; cur < count; cur++) {
    if(menu->items[cur].value) {
      stopChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
  }
  if(!hasPerformedAny) stopChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}

void clearSelectedChronos(Menu* menu, chronometer* tchrono, int count) {
  // do action for each selected timer
  int hasPerformedAny = 0;
  for(int cur = 0; cur < count; cur++) {
    if(menu->items[cur].value) {
      clearChrono(&tchrono[cur]);
      hasPerformedAny = 1;
    }
  }
  if(!hasPerformedAny) clearChrono(&tchrono[menu->selection-1]); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO); 
}



void setChronoGUI(Menu* menu, chronometer* tchrono) {
  long long int ms = 0;
  int type = CHRONO_TYPE_UP;
  MenuItem menuitems[10];
  menuitems[0].text = (char*)"Upwards";
  menuitems[1].text = (char*)"Downwards (period)";
  menuitems[2].text = (char*)"Downwards (date-time)";
  
  Menu bmenu;
  bmenu.items=menuitems;
  bmenu.numitems=3;
  bmenu.height = 4;
  bmenu.scrollbar=0;
  bmenu.title = (char*)"Set chronometer type";
  
  textArea text;
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.y = 4*24+5;

  textElement elem[2];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"Chronometers will start the moment you set them. You should familiarize yourself with the behavior of the chronometer function before using it for something important.";

  text.numelements = 1;
  Bdisp_AllClr_VRAM();
  
  while(1) {
    doTextArea(&text);
    int res = doMenu(&bmenu);
    if(res == MENU_RETURN_EXIT) {
      Bdisp_AllClr_VRAM();
      return;
    } else if(res == MENU_RETURN_SELECTION) {
      if(bmenu.selection == 1) {
        type=CHRONO_TYPE_UP;
        break;
      } else if(bmenu.selection == 2) {
        Selector sel;
        sel.title =  (char*)"Set downwards chrono.";
        sel.subtitle = (char*)"Days";
        sel.max = -1; // no limit. long long int is big enough to accomodate a chronometer with a duration of over 2 million days.
        sel.cycle = 0;
        sel.type = SELECTORTYPE_NORMAL;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        long int days = sel.value;
        
        sel.subtitle = (char*)"Hours";
        sel.max = 23;
        sel.value = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int hours = sel.value;
        
        sel.subtitle = (char*)"Minutes";
        sel.max = 59;
        sel.value = 0;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        int minutes = sel.value;
        
        sel.subtitle = (char*)"Seconds";
        // yes, we are assigning the truth value to two vars at once:
        sel.value = sel.min = (days == 0 && hours == 0 && minutes == 0);
        sel.max = 59;
        if (doSelector(&sel) == SELECTOR_RETURN_EXIT) return;
        ms = 1000 * (sel.value + 60*minutes + 60*60*hours + 60*60*24*days);
        type=CHRONO_TYPE_DOWN;
        break;
      } else if(bmenu.selection == 3) {
        int y=getCurrentYear(), m=getCurrentMonth(), d=getCurrentDay();
        if(chooseCalendarDate(&y, &m, &d, (char*)"Select chronometer end date", (char*)"", 1)) return;
        if(DateToDays(y, m, d) - DateToDays(getCurrentYear(), getCurrentMonth(), getCurrentDay()) < 0) {
          mMsgBoxPush(4);
          mPrintXY(3, 2, (char*)"Date is in the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 3, (char*)"past.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          closeMsgBox();
          return;
        }
        Bdisp_AllClr_VRAM();
        drawScreenTitle((char*)"Set downwards chrono.", (char*)"Chronometer end time:");
        mPrintXY(8, 4, (char*)"HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        
        textInput input;
        input.x=8;
        input.width=6;
        input.charlimit=6;
        input.acceptF6=1;
        input.type=INPUTTYPE_TIME;
        char etbuffer[15];
        etbuffer[0] = 0;
        input.buffer = (char*)etbuffer;
        int h=0,mi=0,s=0;
        while(1) {
          input.key=0;
          int res = doTextInput(&input);
          if (res==INPUT_RETURN_EXIT) return; // user aborted
          else if (res==INPUT_RETURN_CONFIRM) {
            if((int)strlen(etbuffer) == input.charlimit) {
                stringToTime(etbuffer, &h, &mi, &s);
                if(isTimeValid(h, mi, s)) {
                  break;
                } else invalidFieldMsg(1);
            } else {
              invalidFieldMsg(1);
            }
          } 
        }
        ms = DateTime2Unix(y, m, d, h, mi, s, 0) - currentUnixTime();
        if(ms < 0) {
          mMsgBoxPush(4);
          mPrintXY(3, 2, (char*)"Time is in the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          mPrintXY(3, 3, (char*)"past.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          closeMsgBox();
          return;
        }
        type=CHRONO_TYPE_DOWN;
        break;
      }
    }
  }
  
  int hasPerformedAny = 0;
  for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      setChrono(&tchrono[cur], ms, type);
      hasPerformedAny=1;
    }
  }
  if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], ms, type); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO);
}

void setBuiltinChrono(Menu* menu, chronometer* tchrono) {
  long long int duration = 0;
  MenuItem menuitems[10];
  menuitems[0].text = (char*)"1 minute timer";  
  menuitems[1].text = (char*)"5 minutes timer";  
  menuitems[2].text = (char*)"15 minutes timer";  
  menuitems[3].text = (char*)"30 minutes timer";
  menuitems[4].text = (char*)"1 hour timer";
  menuitems[5].text = (char*)"1 hour 30 min. timer";
  menuitems[6].text = (char*)"2 hours timer";
  menuitems[7].text = (char*)"5 hours timer";
  menuitems[8].text = (char*)"12 hours timer";
  menuitems[9].text = (char*)"1 day timer";
  
  Menu bmenu;
  bmenu.items=menuitems;
  bmenu.numitems=10;
  bmenu.scrollout=1;
  while(1) {
    int res = doMenu(&bmenu);
    if(res == MENU_RETURN_EXIT) {
      Bdisp_AllClr_VRAM();
      return;
    } else if(res == MENU_RETURN_SELECTION) {
      switch(bmenu.selection) {
        case 1: duration = 60*1000; break;
        case 2: duration = 5*60*1000; break;
        case 3: duration = 15*60*1000; break;
        case 4: duration = 30*60*1000; break;
        case 5: duration = 60*60*1000; break;
        case 6: duration = 90*60*1000; break;
        case 7: duration = 120*60*1000; break;
        case 8: duration = 5*60*60*1000; break;
        case 9: duration = 12*60*60*1000; break;
        case 10: duration = 24*60*60*1000; break;
      }
      break;
    }
  }
  
  int hasPerformedAny = 0;
  for(int cur = 0; cur < NUMBER_OF_CHRONO; cur++) {
    if(menu->items[cur].value == MENUITEM_VALUE_CHECKED) {
      setChrono(&tchrono[cur], duration, CHRONO_TYPE_DOWN);
      hasPerformedAny=1;
    }
  }
  if(!hasPerformedAny) setChrono(&tchrono[menu->selection-1], duration, CHRONO_TYPE_DOWN); // if there was no selected chrono, do it for the currently selected menu position
  saveChronoArray(tchrono, NUMBER_OF_CHRONO);
  Bdisp_AllClr_VRAM();
}

int getLastChronoComplete() { return lastChronoComplete; }

void checkDownwardsChronoCompleteGUI(chronometer* chronoarray, int count) {
  for(int cur = 0; cur < count; cur++) {
    if(chronoarray[cur].state == CHRONO_STATE_RUNNING && chronoarray[cur].type == CHRONO_TYPE_DOWN && //...
    // check if chrono is complete
    // if end time of chrono (start+duration) <= current time and chrono is running, chrono is complete
    /*...*/  chronoarray[cur].starttime+chronoarray[cur].duration<=currentUnixTime()) {
      //clear this chrono
      clearChrono(&chronoarray[cur]);
      saveChronoArray(chronoarray, NUMBER_OF_CHRONO);
      lastChronoComplete = cur+1; // lastChronoComplete is one-based
      if(GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) && GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) != 3) {
        // user wants notification with pop-up
        mMsgBoxPush(4);
        char buffer1[10];
        itoa(cur+1, (unsigned char*)buffer1);
        mPrintXY(3,2,(char*)"Chronometer ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(15,2,(char*)buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        mPrintXY(3,3,(char*)"ended.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        
        if(GetSetting(SETTING_CHRONO_NOTIFICATION_TYPE) == 1) {
          // notification with screen flashing
          PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
          flashLight(1); // with parameter set to 1, it doesn't change VRAM, and since it returns on pressing EXIT...
          mMsgBoxPop();
        } else {
          // without screen flashing
          closeMsgBox();
        }
        lastChronoComplete = 0; // otherwise, notification may show on home for a previous timer, if the user changes the setting in the meantime.
      }
    }
  }
}