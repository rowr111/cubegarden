#ifndef __ORCHARD_H__
#define __ORCHARD_H__

extern const char *gitversion;
extern struct evt_table orchard_events;

void halt(void);

#define ORCHARD_OS_VERSION_MAJOR      1
#define ORCHARD_OS_VERSION_MINOR      0

#define serialDriver                  (&SD4)
#define stream_driver                 ((BaseSequentialStream *)serialDriver)
#define radioDriver                   (&KRADIO1)

#define stream (BaseSequentialStream *)&SD4

//extern void *stream;

#define i2cDriver                     (&I2CD1)
#define accelAddr                     0x1c

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#endif

#endif /* __ORCHARD_H__ */
