#ifndef __TEXTGUI_H
#define __TEXTGUI_H
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

typedef struct
{
  char* text;
  short newLine=0; // if 1, new line will be drawn before the text
  color_t color=COLOR_BLACK;
  short spaceAtEnd=0;
  short lineSpacing=0;
  short minimini=0;
} textElement;

#define TEXTAREATYPE_NORMAL 0
#define TEXTAREATYPE_INSTANT_RETURN 1
typedef struct
{
  short x=0;
  short y=0;
  short width=LCD_WIDTH_PX;
  short lineHeight=17;
  textElement* elements;
  int numelements;
  char title[42];
  short showtitle = 1;
  short scrollbar=1;
  short allowEXE=0; //whether to allow EXE to exit the screen
  short allowF1=0; //whether to allow F1 to exit the screen
  short type=TEXTAREATYPE_NORMAL;
} textArea;

#define TEXTAREA_RETURN_EXIT 0
#define TEXTAREA_RETURN_EXE 1
#define TEXTAREA_RETURN_F1 2
short doTextArea(textArea* text); //returns 0 when user EXITs, 1 when allowEXE is true and user presses EXE, 2 when allowF1 is true and user presses F1.

#endif 
