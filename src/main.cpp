/*
 * Il mastro Stefanuzzo <ilmastrostefanuzzo@gmail.com>
 * https://github.com/ilmastrostefanuzzo
 * MIT license
 */

#include <Arduino.h>

/* Serial debug */
// When debugging, the program will print to serial instead of sending the actual keypresses
// When not debugging, nothing will be printed to serial as that would mess with the hacky way HID works on the Arduino UNO
// #define SERIAL_DEBUG

#ifdef SERIAL_DEBUG
  #define print_mode() Serial.println("MODE: " + String(mode, DEC));
#else
  #define print_mode()
#endif
/* Serial debug */

/* IRMP library */
#ifdef SERIAL_DEBUG
  // Allows the name (eg: SAMSG32) of a protocol instead of its number (eg: 0xA) to be printed when debugging. Disabling this in production saves some flash
  #define IRMP_PROTOCOL_NAMES 1
#endif
// Enables a specific protocol, see https://github.com/ukw100/IRMP/blob/master/src/irmpSelectAllProtocols.h for a list of available protocols
#define IRMP_SUPPORT_SAMSUNG_PROTOCOL 1
/* Use one of these to figure out which protocol(s) you need or if you want to just enable everything
#include <irmpSelectAllProtocols.h>
#include <irmpSelectMain15Protocols.h>
*/
#define IRMP_INPUT_PIN A0 // IR input pin
#include <irmp.hpp>
/* IRMP library */

/* Key mappings */
// Contains all the information needed for one key association
typedef struct {
  uint32_t ir_code;        // IR code to be detected
  uint8_t kb_code;         // Keyboard HID code to be sent
  bool is_repeatable;      // Will holding down be ignored?
  String serial_debug_msg; // Debug message
} Mapping;

// Contains definitions for all HID keyboard codes so KEY_A can be used instead of 0x04
#include "kb_hid_codes.h"
// I think you need to do this instead if you're using the Arduino IDE:
// #include "include/kb_hid_codes.h"

#define MODAL_KEY 0xB946  // Goes to the next mode or 0 after reaching the end
#define MODE_0_KEY 0xA758 // This key sets the mode to 0
#define MODES 3           // Number of available modes
const Mapping key_mappings[MODES][4] = {

    // Mode 0: Main mode
    {{0xBA45, KEY_SPACE, 0, "Play/Pause (Space)"},
     {0xB847, KEY_MUTE, 0, "Mute"},
     {0xB54A, KEY_VOLUMEDOWN, 1, "Volume down"},
     {0xB748, KEY_VOLUMEUP, 1, "Volume up"}},

    // Mode 1: Fullscreen and video navigation
    {{0xBA45, KEY_F, 0, "F"},
     {0xB847, KEY_0, 0, "0"},
     {0xB54A, KEY_J, 1, "J"},
     {0xB748, KEY_L, 1, "L"}},

    // Mode 2: Video navigation by number
    {{0xBA45, KEY_1, 0, "1"},
     {0xB847, KEY_3, 0, "3"},
     {0xB54A, KEY_6, 0, "6"},
     {0xB748, KEY_9, 0, "9"}},

};
/* Key mappings */

/* Declare functions which are defined at the bottom for readability */
void releaseKey();
void sendKey(const uint8_t key, const String serial_debug_msg);

// * Setup
uint8_t buf[8]; // Keyboard report buffer
uint8_t mode = 0;
IRMP_DATA irmp_data;
void setup() {
  Serial.begin(9600);

  irmp_init();
}

// * Main loop
void loop() {
  if (irmp_get_data(&irmp_data)) {

#ifdef SERIAL_DEBUG
    Serial.println();
    irmp_result_print(&irmp_data);
#endif

    bool didnt_repeat = !(irmp_data.flags & IRMP_FLAG_REPETITION);

    // Modal and mode 0 keys
    if ((irmp_data.command == MODAL_KEY) & didnt_repeat) {
      ++mode %= MODES;
      print_mode();
    } else if (irmp_data.command == MODE_0_KEY) {
      mode = 0;
      print_mode();
    }

    // Keys defined in key_mappings[]
    else {
      for (const auto &key : key_mappings[mode]) {
        if ((key.ir_code == irmp_data.command) &
            (didnt_repeat || key.is_repeatable)) {
          sendKey(key.kb_code, key.serial_debug_msg);
          break;
        }
      }
    }
  }
}

// The first argument is the code for the keypress, the second one is the
// message to print to serial when debugging
void sendKey(const uint8_t key, const String serial_debug_msg) {
#ifndef SERIAL_DEBUG
  buf[2] = key;
  Serial.write(buf, 8); // Send keypress
  releaseKey();
#else
  Serial.println("PROCESSED KEY: " + serial_debug_msg);
#endif
}

void releaseKey() {
  buf[0] = 0;
  buf[2] = 0;
  Serial.write(buf, 8);
}
