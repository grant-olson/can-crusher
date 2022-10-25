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
  PROP_LEN,
  PROP_UNKNOWN=-1
} can_prop;

int property_init();
can_prop property_get_prop_id(const char* prop_name);
uint32_t property_get_prop(can_prop prop);
int property_set_prop(can_prop prop, uint32_t value);
uint32_t property_get_prop_by_name(const char* prop_name);

#endif
