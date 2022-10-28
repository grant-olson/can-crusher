#ifndef __PROPERTIES_H__
#define __PROPERTIES_H__

typedef enum can_prop {
  PROP_TCOOL_THRESHOLD,
  PROP_STALLGUARD_THRESHOLD,
  PROP_STALLGUARD_ENABLED,
  PROP_STEPS_PER_MM,
  PROP_SUBSTEPS_PER_STEP,
  PROP_HOME_SPEED,
  PROP_HOME_RETRACT_MM,
  PROP_MOTOR_NOT_HOMED,
  PROP_LEN,
  PROP_UNKNOWN=-1
} can_prop;

// Save in the very last sector possible
#define PROP_OFFSET (1024 * 1024 * 2) - FLASH_SECTOR_SIZE
#define PROP_ADDRESS ( XIP_BASE + PROP_OFFSET)

//
// The structure for the data saved in flash is:
// uint32_t marker = 0xaffab1e0
// uint32_t version;
// uint32_t props[PROP_LEN];

// "AFFIABLE" in leet-speak, little-endian bytes
// so we don't need to do lame bit shifting.
#define PROP_MARKER 0xaffab1e0
#define PROP_MARKER_B0 0xe0
#define PROP_MARKER_B1 0xb1
#define PROP_MARKER_B2 0xfa
#define PROP_MARKER_B3 0xaf

#define PROP_VERSION 0x1

int property_init();
can_prop property_get_prop_id(const char* prop_name);
uint32_t property_get_prop(can_prop prop);
int property_set_prop(can_prop prop, uint32_t value);
uint32_t property_get_prop_by_name(const char* prop_name);

void property_set_defaults();
void property_load();
void property_save();

#endif
