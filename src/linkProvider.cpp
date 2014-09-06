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

#include "linkProvider.hpp"
#include "menuGUI.hpp"
#include "graphicsProvider.hpp"
#include "textGUI.hpp"

// code based on example by Simon Lothar
// send a file through the 3-pin serial. Compatible with Casio's functions.
// remote calculator should be in receive mode with cable type set to 3-pin
// pay attention to CPU clock speeds as they affect the baud rate, causing errors

void endSerialComm(int error) {
  if(error) {
    Comm_Terminate(2); // display "Receive ERROR" on remote
  } else {
    Comm_Terminate(0); // display "Complete!" on remote
  }
  while(1) {
    // wait for pending transmissions and close serial
    if(Comm_Close(0) != 5) break;
  }

  mMsgBoxPush(4);
  if(error) {
    mPrintXY(3, 2, (char*)"An error", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    char buffer[20];
    char buffer2[20];
    strcpy(buffer, "occurred (");
    itoa(error, (unsigned char*)buffer2);
    strcat(buffer, buffer2);
    strcat(buffer, ").");
    mPrintXY(3, 3, buffer, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    mPrintXY(3, 2, (char*)"Transfer", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    mPrintXY(3, 3, (char*)"successful.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  closeMsgBox();
}

void serialTransferSingleFile(char* filename) {
  if(strncmp(filename, "\\\\fls0\\", 7)) return; // ERROR

  textArea text;
  text.type=TEXTAREATYPE_INSTANT_RETURN;
  text.scrollbar=0;
  text.title = (char*)"Sending file...";
  
  textElement elem[15];
  text.elements = elem;
  text.numelements = 0; //we will use this as element cursor
  
  elem[text.numelements].text = (char*)"Preparing...";
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  TTransmitBuffer sftb;

  memset(&sftb, 0, sizeof(sftb));
  strcpy(sftb.device, "fls0");
  strcpy(sftb.fname1, (char*)filename+7);

  Bfile_StrToName_ncpy(sftb.filename, filename, 0x10A);
  // get filesize:
  int handle = Bfile_OpenFile_OS(sftb.filename, READWRITE, 0);
  sftb.filesize = Bfile_GetFileSize_OS(handle);
  Bfile_CloseFile_OS(handle);

  sftb.command = 0x45;
  sftb.datatype = 0x80;
  sftb.handle = -1;
  sftb.source = 1; // SMEM:1
  sftb.zero = 0;

  // start communicating with remote
  App_LINK_SetReceiveTimeout_ms(6000);
  if(Comm_Open(0x1000)) {
    endSerialComm(1);
    return;
  }

  elem[text.numelements].text = (char*)"Checking connection...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(Comm_TryCheckPacket(0)) {
    endSerialComm(2);
    return;
  }

  elem[text.numelements].text = (char*)"Changing connection settings...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(App_LINK_SetRemoteBaud()) {
    endSerialComm(3);
    return;
  }

  OS_InnerWait_ms(20); // wait for remote calculator to change link settings

  elem[text.numelements].text = (char*)"Checking new settings...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(Comm_TryCheckPacket(1)) {
    endSerialComm(4);
    return;
  }

  // get information about remote calculator

  elem[text.numelements].text = (char*)"Getting remote info...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  int ret = App_LINK_Send_ST9_Packet();
  if(ret && (ret != 0x14)) {
    endSerialComm(5);
    return;
  }

  unsigned int calcType;
  unsigned short osVer;
  if(App_LINK_GetDeviceInfo(&calcType, &osVer)) {
    endSerialComm(6);
    return;
  }

  // do something with calcType and osVer

  elem[text.numelements].text = (char*)"Preparing file sending...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  // start file transfer
  if(App_LINK_TransmitInit(&sftb)) {
    endSerialComm(7);
    return;
  }

  elem[text.numelements].text = (char*)"Sending file...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  if(App_LINK_Transmit(&sftb)) {
    endSerialComm(8);
    return;
  }

  elem[text.numelements].text = (char*)"Ending connection...";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  doTextArea(&text);
  Bdisp_PutDisp_DD();

  // terminate transfer and close serial
  endSerialComm(0);

  elem[text.numelements].text = (char*)"Done, press EXIT";
  elem[text.numelements].newLine = 1;
  text.numelements++;
  text.type = TEXTAREATYPE_NORMAL;
  doTextArea(&text);

  // done!
  return;
}

int SerialFileTransfer(char*_filename ) {
  TTransmitBuffer sftb;
  int l = strlen( (char*)_filename );
  unsigned int iii, jjj, phase;
  //FILE_INFO fileinfo;
  unsigned int calcType;
  unsigned short osVer;

  if ( l <= 7 ) return -1;
  if ( _filename[0] != '\\' ) return -2;
  if ( _filename[1] != '\\' ) return -3;
  if ( _filename[6] != '\\' ) return -4;

  memset( &sftb, 0, sizeof( sftb ) );

  strncpy( sftb.device, (char*)_filename+2, 4 );
  if ( strcmp( sftb.device, "fls0" ) ) return -5;

  strcpy( sftb.fname1, (char*)_filename+7 );
  // strcpy( sftb.fname2, (char*)_filename+7 );

  Bfile_StrToName_ncpy( sftb.filename, _filename, 0x10A );
  // get filesize:
  int handle = Bfile_OpenFile_OS(sftb.filename, READWRITE, 0);
  sftb.filesize = Bfile_GetFileSize_OS(handle);
  Bfile_CloseFile_OS(handle);

  sftb.command = 0x45;
  sftb.datatype = 0x80;
  sftb.handle = -1;
  sftb.source = 1; // SMEM:1
  sftb.zero = 0;

  phase = 0;

  while( 1 ){
    phase = 1;
    //App_LINK_SetReceiveTimeout_ms( 360000 );
    App_LINK_SetReceiveTimeout_ms( 6000 );
    iii = Comm_Open( 0x1000 );
    if ( iii ) break;

    phase = 2;
    // Serial_Send_0Ax10_Sequence();
    //SetGetKeyPhaseFlag(); // 02CA
    iii = Comm_TryCheckPacket( 0 ); // 1396
    if ( iii ) break;

    phase = 3;
    iii = App_LINK_SetRemoteBaud(); // 1397
    if ( iii ) break;

    OS_InnerWait_ms( 20 );

    phase = 4;
    iii = Comm_TryCheckPacket( 1 ); // 1396
    if ( iii ) break;

    phase = 5;
    iii = App_LINK_Send_ST9_Packet();
    if ( iii && ( iii != 0x14 ) ) break;

    phase = 6;
    iii = App_LINK_GetDeviceInfo( &calcType, &osVer );
    if ( iii ) break;

    phase = 7;
    iii = App_LINK_TransmitInit( &sftb );
    //iii = App_LINK_TransmitInit2( &sftb, OVERWRITE );
    if ( iii ) break;

    phase = 8;
    iii = App_LINK_Transmit( &sftb );
    if ( iii ) break;

    phase = 0xFF;
    break;
  };
  Comm_Terminate( 0 );
  jjj = 1;
  while ( Comm_Close( 0 ) == 5 ) jjj++;

  phase<<=24;
  jjj<<=16;
  return phase + jjj + iii;
}