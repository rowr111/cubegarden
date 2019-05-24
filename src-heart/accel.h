#ifndef __ORCHARD_ACCEL_H__
#define __ORCHARD_ACCEL_H__

struct accel_data {
  int x;
  int y;
  int z;
};

void accel_irq(EXTDriver *extp, expchannel_t channel);
void accel_proc(eventid_t id);

void accelStart(I2CDriver *driver);
void accelStop(void);
msg_t accelPoll(struct accel_data *data);
void accelEnableFreefall(int sensitivity, int debounce);
void accelDisableFreefall(void);

extern event_source_t accel_x_axis_pulse;
extern event_source_t accel_y_axis_pulse;
extern event_source_t accel_z_axis_pulse;
extern event_source_t accel_landscape_portrait;
extern event_source_t accel_freefall;
extern event_source_t accel_process;

#endif /* __ORCHARD_ACCEL_H__ */
