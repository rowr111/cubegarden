#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include <stdlib.h>
#include "shellcfg.h"

#include "barometer.h"

#if 0
void baroCmd(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void) argc;
  (void) argv;
  
  float temperature;
  float pressure;
  int16_t oversampling = 7;
  int16_t ret;

  ret = baro_measureTempOnce(&temperature, oversampling);

  if (ret != 0)  {
    chprintf(chp, "FAIL! ret = %d\n\r", ret);
  } else {
    chprintf(chp, "Temperature: %0.2f C\n\r", temperature);
  }

  ret = baro_measurePressureOnce(&pressure, oversampling);
  if (ret != 0)  {
    chprintf(chp, "FAIL! ret = %d\n\r", ret);
  }  else  {
    chprintf(chp, "Pressure: %0.2f Pascal\n\r", pressure);
  }

}
orchard_shell("baro", baroCmd);
#else

void baroCmd(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void) argc;
  (void) argv;

  // barometer measurements are automatically sampled by a background thread
  // to avoid too much additional latency during effects processing

  chprintf(chp, "Temperature: %0.2f C\n\r", baro_temp);
  chprintf(chp, "Pressure: %0.2f Pascal\n\r", baro_pressure);
  chprintf(chp, "Avg pressure: %0.2f Pascal over %d samples; valid: %d\n\r", baro_avg, BARO_HISTORY, baro_avg_valid);

}
#endif

orchard_shell("baro", baroCmd);
