/*********************
 * extui_bigtreetech_tft.cpp *
 *********************/

/****************************************************************************
 *   Written By Lucio Tarantino 2019                                        *
 *                                                                          *
 *   This program is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   To view a copy of the GNU General Public License, go to the following  *
 *   location: <http://www.gnu.org/licenses/>.                              *
 ****************************************************************************/

#include "../inc/MarlinConfigPre.h"

#if ENABLED(BIGTREETECH_TFT_LCD)

#include "extensible_ui/ui_api.h"
#include "../feature/host_actions.h"

#if ENABLED(SDSUPPORT)
  #include "../sd/cardreader.h"
  #include "../sd/SdFatConfig.h"
#else
  #define LONG_FILENAME_LENGTH 0
#endif

#define MAX_TFT_COMMAND (32 + LONG_FILENAME_LENGTH) * 2

// See http://docs.octoprint.org/en/master/features/action_commands.html for Spec.
void write_tft_action_command(const char *fmt, ...){
  char logrow[MAX_TFT_COMMAND];
  va_list args; 
  va_start(args,*fmt);
  vsprintf(logrow,fmt,args);
  va_end(args);
  PORT_REDIRECT(TFT_SERIAL);
  host_action(logrow, true);
  PORT_RESTORE();

//  TFT35_SERIAL.Print::printf("//action:%s\n",logrow);
}

// Supported Action https://reprap.org/wiki/G-code#Action_commands
// Original Octoprint Action Commands
//#define ACTION_PAUSE            "pause"                      // pause current print
//#define ACTION_CANCEL           "cancel"                     // cancel the current printer job ( a message is optional )
//#define ACTION_DISCONNET        "disconnect"                 // disconnect
//#define ACTION_PAUSED           "paused"                     // the job is paused        
//#define ACTION_RESUME           "resume"                     // resume the job
//#define ACTION_OUT_OF_FILAMENT  "out_of_filament T%d"        // out of filement on %d extruder_id
//#define ACTION_RESUMED          "resumed"                    // the job is resumed

// Dialog commands  (https://reprap.org/wiki/G-code#M876:_Dialog_handling)
//#define ACTION_PROMPT_BEGIN  "prompt_begin %s"  // Starts the definition of a prompt dialog. <message> is the message to display to the user. (support: OctoPrint 1.3.9+ w/ enabled Action Command Prompt support plugin)
//#define ACTION_PROMPT_CHOICE "prompt_choice %s" // Defines a dialog choice with the associated <text>. (support: OctoPrint 1.3.9+ w/ enabled Action Command Prompt support plugin)
//#define ACTION_PROMPT_BUTTON "prompt_button %s" // Same as prompt_choice. (support: OctoPrint 1.3.9+ w/ enabled Action Command Prompt support plugin)
//#define ACTION_PROMPT_SHOW   "prompt_show"      // Tells the host to prompt the user with the defined dialog. (support: OctoPrint 1.3.9+ w/ enabled Action Command Prompt support plugin)
//#define ACTION_PROMPT_END    "prompt_end"       // Tells the host to close the dialog (e.g. the user made the choice through the printer directly instead of through the host). (support: OctoPrint 1.3.9+ w/ enabled Action Command Prompt support plugin)
//#define ACTION_PROBE_REWIPE  "probe_rewipe"     // Displays dialog indicating G29 Probing is Retrying. (support: Lulzbot Cura 3.6+)
//#define ACTION_PROBE_FAILED  "probe_failed"     // Cancels print job and displays dialog indicating G29 Probing failed (support: Lulzbot Cura 3.6+)

// New Action Commands define for the LCD.
#define ACTION_CONNECT   "connect"            // Tell to LCD that Marlin is ready to send action command
//#define ACTION_NOOP      "noop"               // KeepAlive message
#define ACTION_MEDIA      "media"              // Inform on media change ( sub command are insert/remove/error)
#define ACTION_BELL       "bell F%d D%d"               // Bell or visual bell ( frequency duration) 
#define ACTION_START      "start"              // Print start 
#define ACTION_STATUS     "status %s"             // Change status event 
#define ACTION_PROGRESS   "progress S%d P%d"             // Change printing progress (seconds, percent)


namespace ExtUI {

  static uint32_t lastSendProgress = 0; 

  void onStartup() {
    write_tft_action_command(ACTION_CONNECT " Marlin " SHORT_BUILD_VERSION);
  }

  void onIdle() { 
    if(ExtUI::isPrinting() && ExtUI::getProgress_seconds_elapsed() > 1){
    #ifdef LCD_SET_PROGRESS_MANUALLY
      if(safe_millis() - lastSendProgress > 50000){
        write_tft_action_command(ACTION_PROGRESS,ExtUI::getProgress_seconds_elapsed(),ExtUI::getProgress_percent());
        lastSendProgress = safe_millis();
      }
    #endif // LCD_SET_PROGRESS_MANUALLY
    }
  }

  void onPrinterKilled(PGM_P const msg) {
    PORT_REDIRECT(TFT_SERIAL);
    host_prompt_do(PROMPT_INFO, msg, nullptr);
    host_action_kill();
    PORT_RESTORE();
  }

  void onMediaInserted() {
    write_tft_action_command(ACTION_MEDIA" insert");
  };

  void onMediaError() {
    write_tft_action_command(ACTION_MEDIA" error");
  };

  void onMediaRemoved() {
    write_tft_action_command(ACTION_MEDIA" removed");
  };

  void onPlayTone(const uint16_t frequency, const uint16_t duration) {
    write_tft_action_command(ACTION_BELL,frequency, duration);
  }

  void onPrintTimerStarted() {
    write_tft_action_command(ACTION_START);
    write_tft_action_command(ACTION_PROGRESS,ExtUI::getProgress_seconds_elapsed(),ExtUI::getProgress_percent());
  }

  void onPrintTimerPaused() {
    PORT_REDIRECT(TFT_SERIAL);
    host_action_paused();
    PORT_RESTORE();
  }

  void onPrintTimerStopped() {
    PORT_REDIRECT(TFT_SERIAL);
    host_action_cancel();
    PORT_RESTORE(); 
  }

  void onFilamentRunout(ExtUI::extruder_t) {
    #ifdef BIGTREETECH_ONLY_HOST_PROMPT_SUPPORT
      PORT_REDIRECT(TFT_SERIAL);
    #else
      PORT_REDIRECT(SERIAL_BOTH);
    #endif
    // Host Prompt Redirect request on display. Not need any other action.
  }

  void onUserConfirmRequired(const char * const msg) {
    #ifdef BIGTREETECH_ONLY_HOST_PROMPT_SUPPORT
      PORT_REDIRECT(TFT_SERIAL);
    #else
      PORT_REDIRECT(SERIAL_BOTH);
    #endif
    // Host Prompt Redirect request on display. Not need any other action.
  }

  void onStatusChanged(const char * const msg) {
    onIdle();
    write_tft_action_command(ACTION_STATUS,msg);
  }

  void onFactoryReset() {
    PORT_REDIRECT(TFT_SERIAL);
    write_tft_action_command(ACTION_STATUS,MSG_SERVICE_RESET);
    PORT_RESTORE();
  }

  void onStoreSettings(char *buff) {
    // This is called when saving to EEPROM (i.e. M500). If the ExtUI needs
    // permanent data to be stored, it can write up to eeprom_data_size bytes
    // into buff.

    // Example:
    //  static_assert(sizeof(myDataStruct) <= ExtUI::eeprom_data_size);
    //  memcpy(buff, &myDataStruct, sizeof(myDataStruct));
    
    write_tft_action_command(ACTION_STATUS,MSG_STORE_EEPROM);
  }

  void onLoadSettings(const char *buff) {
    // This is called while loading settings from EEPROM. If the ExtUI
    // needs to retrieve data, it should copy up to eeprom_data_size bytes
    // from buff

    // Example:
    //  static_assert(sizeof(myDataStruct) <= ExtUI::eeprom_data_size);
    //  memcpy(&myDataStruct, buff, sizeof(myDataStruct));
    write_tft_action_command(ACTION_STATUS,MSG_LOAD_EEPROM);
  }

  void onConfigurationStoreWritten(bool success) {
    // This is called after the entire EEPROM has been written,
    // whether successful or not.
  }

  void onConfigurationStoreRead(bool success) {
    // This is called after the entire EEPROM has been read,
    // whether successful or not.
  }
}

#endif // BIGTREETECH_TFT_LCD
