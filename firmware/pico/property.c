#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "string.h"
#include "motor.h"
#include "property.h"

static uint32_t property_values[PROP_LEN];

void property_set_defaults() {
  property_values[PROP_TCOOL_THRESHOLD] = 0xFFFFF;
  property_values[PROP_STALLGUARD_THRESHOLD] = 0x60;
  property_values[PROP_STALLGUARD_ENABLED] = 0xFFFFFFFF;
  property_values[PROP_STEPS_PER_MM] = 25;
  property_values[PROP_SUBSTEPS_PER_STEP] = 8;
}

int property_init() {
  property_set_defaults();

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
  }

  return ret;
}

uint32_t property_get_prop_by_name(const char* prop_name) {
  can_prop prop = property_get_prop_id(prop_name);
  return property_values[prop];
}
