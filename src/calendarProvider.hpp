#ifndef __CALENDARPROVIDER_H
#define __CALENDARPROVIDER_H

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

#define MAX_DAY_EVENTS 100 //max events in one day
#define MAX_DAY_EVENTS_WEEKVIEW 100 //max same-day events to load in week view
#define MAX_EVENT_FILESIZE 50000

#define FILE_HEADER "PCALEVT" //Prizm CALendar EVenT / Portable CALendar EVenT
#define FIELD_SEPARATOR '\x1D'
#define EVENT_SEPARATOR '\x1E'

typedef struct // Event date definition
{
  unsigned short day=0;
  unsigned short month=0;
  unsigned short year=0;
} EventDate;

typedef struct // Event time definition
{
  unsigned short hour=0;
  unsigned short minute=0;
  unsigned short second=0;
} EventTime;

typedef struct // Defines what a calendar event contains
{
  unsigned short category=0;
  //unsigned short daterange=0; //not used for now
  EventDate startdate;
  EventDate enddate;
  //unsigned short dayofweek=1; // not used for now
  unsigned short repeat=0;
  unsigned short timed=1; //full-day = 0, timed = 1
  EventTime starttime;
  EventTime endtime;
  unsigned char title[25]; //can't be 21, because otherwise somehow the location will replace the last chars of title
  unsigned char location[135]; //can't be 128, because otherwise somehow the description may flow into the location.
  unsigned char description[1030]; //orig 1024
} CalendarEvent;

typedef struct // a simplified calendar event, for use when the memory available is little and one needs to show many events
{
  unsigned short category=0;
  EventDate startdate;
  unsigned char title[25];
  unsigned short origpos=0; //position in original file (useful for search results). zero based.
} SimpleCalendarEvent;
// end of type definitions

void calEventToChar(CalendarEvent* calEvent, unsigned char* buf);
void charToCalEvent(unsigned char* src, CalendarEvent* calEvent);
void charToSimpleCalEvent(unsigned char* src, SimpleCalendarEvent* calEvent);
void filenameFromDate(EventDate* date, char* filename);
void smemFilenameFromDate(EventDate* date, unsigned short* filename, const char* folder);
short AddEvent(CalendarEvent* calEvent, const char* folder, short secondCall=0);
short ReplaceEventFile(EventDate *startdate, CalendarEvent* newEvents, const char* folder, short count);
void RemoveEvent(EventDate *startdate, CalendarEvent* events, const char* folder, short count, short calEventPos);
void RemoveDay(EventDate* date, const char* folder);
short GetEventsForDate(EventDate* startdate, const char* folder, CalendarEvent* calEvents, short limit=0, SimpleCalendarEvent* simpleCalEvents = NULL, short startArray=0);
void GetEventCountsForMonth(short year, short month, short* buffer, short* busydays);
short SearchEventsOnDay(EventDate* date, const char* folder, SimpleCalendarEvent* calEvents, char* needle, short limit);
short SearchEventsOnMonth(short y, short m, const char* folder, SimpleCalendarEvent* calEvents, char* needle, short limit);
short SearchEventsOnYearOrMonth(short y, short m, const char* folder, SimpleCalendarEvent* calEvents, char* needle, short limit, short arraystart=0);
void repairEventsFile(char* name, const char* folder, int* checkedevents, int* problemsfound);
void setDBneedsRepairFlag(short value);
short getDBneedsRepairFlag();

#endif
