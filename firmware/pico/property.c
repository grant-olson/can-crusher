#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "string.h"
#include "motor.h"
#include "property.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

static uint32_t property_values[PROP_LEN];

const uint8_t* flash_bytes = (const uint8_t *) PROP_ADDRESS;
const uint32_t* flash_dwords = (const uint32_t *) PROP_ADDRESS;

void property_set_defaults() {
  property_values[PROP_TCOOL_THRESHOLD] = 0x100;
  property_values[PROP_STALLGUARD_THRESHOLD] = 0x83;
  property_values[PROP_STALLGUARD_ENABLED] = 0xFFFFFFFF;
  property_values[PROP_STEPS_PER_MM] = 25;
  property_values[PROP_SUBSTEPS_PER_STEP] = 8;
  property_values[PROP_HOME_SPEED] = 10;
  property_values[PROP_HOME_RETRACT_MM] = 200;
  property_values[PROP_MOTOR_NOT_HOMED] = (uint32_t)MOTOR_NOT_HOMED;
}

int property_init() {
  if(flash_dwords[0] == PROP_MARKER) {
    property_load();
  } else {
    property_set_defaults();
    property_save();
  }

  return 0;
};

uint32_t property_get_prop(can_prop prop) {
  // Make sure we have something in valid array bounds
  if (prop == PROP_UNKNOWN || prop >= PROP_LEN) { return -1;}

  return property_values[prop];
}

int property_set_prop(can_prop prop, uint32_t value) {
  // Make sure we have something in valid array bounds
  if (prop == PROP_UNKNOWN || prop >= PROP_LEN) { return -1;}

  // read only
  if (prop == PROP_MOTOR_NOT_HOMED) { return -1; }
  
  property_values[prop] = value;

  return 0;
}

can_prop property_get_prop_id(const char* prop_name) {
  can_prop ret = PROP_UNKNOWN;
  
  if (!strcmp(prop_name, "TCOOL_THRESHOLD")) {
    ret = PROP_TCOOL_THRESHOLD;
  } else if (!strcmp(prop_name, "STALLGUARD_THRESHOLD")) {
    ret = PROP_STALLGUARD_THRESHOLD;
  } else if (!strcmp(prop_name, "STALLGUARD_ENABLED")) {
    ret = PROP_STALLGUARD_ENABLED;
  } else if (!strcmp(prop_name, "STEPS_PER_MM")) {
    ret = PROP_STEPS_PER_MM;
  } else if (!strcmp(prop_name, "SUBSTEPS_PER_STEP")) {
    ret = PROP_SUBSTEPS_PER_STEP;
  } else if (!strcmp(prop_name, "HOME_SPEED")) {
    ret = PROP_HOME_SPEED;
  } else if (!strcmp(prop_name, "HOME_RETRACT_MM")) {
    ret = PROP_HOME_RETRACT_MM;
  } else if (!strcmp(prop_name, "MOTOR_NOT_HOMED")) {
    ret = PROP_MOTOR_NOT_HOMED;
  }

  return ret;
}

uint32_t property_get_prop_by_name(const char* prop_name) {
  can_prop prop = property_get_prop_id(prop_name);
  return property_values[prop];
}

void property_load() {
  uint32_t sentinel = flash_dwords[0];
  printf("PROP MARKER: 0x%08x\n", sentinel);
  if(sentinel == PROP_MARKER) {
    printf("GOOD MARKER\n");
  } else {
    printf("BAD MARKER\n");
  }

  uint32_t version = flash_dwords[1];
  printf("VERSION: %d\n", version);
  if (version == PROP_VERSION) {
    printf("GOOD VERSION\n");
  } else {
    printf("BAD VERSION\n");
  }

  // Once we have more than one version, will need to load
  // known values, set defaults, then save.
  int prop_len = PROP_LEN;

  for (int i=0;i<prop_len;i++) {
    property_values[i] = flash_dwords[i+2];
    printf("PROP[%d] = %d\n", i, property_values[i]);
  }
}

void property_save() {
  const uint8_t* flash_bytes = (const uint8_t *) PROP_ADDRESS;
  const uint32_t* flash_dwords = (const uint32_t *) PROP_ADDRESS;

  printf("PERSISTING PROPERTIES\n");
  
  uint8_t data[FLASH_PAGE_SIZE] = {0xFF};
  uint32_t* data_dwords = (uint32_t*) data;
  data[0] = PROP_MARKER_B0;
  data[1] = PROP_MARKER_B1;
  data[2] = PROP_MARKER_B2;
  data[3] = PROP_MARKER_B3;

  data_dwords[1] = PROP_VERSION;

  for(int i=0;i<PROP_LEN;i++) {
    data_dwords[i+2] = property_values[i];
  }

  // Shut off interrupts.
  // We MUST erase, you can't 'program' a bit from 0 back to 1
  // only the other way.
  uint32_t interrupt_status = save_and_disable_interrupts();
  flash_range_erase(PROP_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(PROP_OFFSET, data, FLASH_PAGE_SIZE);
  restore_interrupts(interrupt_status);

  puts("PERSISTED VALUES\n");
  printf("MARKER 0x%08x\n", flash_dwords[0]);
  
  printf("VERSION %d\n", flash_dwords[1]);
  for(int i=0;i<PROP_LEN;i++) {
    printf("PROP[%d] = %d\n", i, flash_dwords[i+2]);
  }
  
}
