#include "ch.h"
#include "hal.h"
#include "led.h"
#include "orchard-effects.h"
#include "gfx.h"

#include "chprintf.h"
#include "stdlib.h"
#include "orchard-math.h"
#include "fixmath.h"

#include "genes.h"
#include "storage.h"
#include "time.h"

#include "orchard-test.h"
#include "test-audit.h"

#include <string.h>
#include <math.h>

orchard_effects_end();

extern void ledUpdate(uint8_t *fb, uint32_t len);

// static (master-local) effects state
led_config_def  led_config;
static effects_config fx_config;
static uint8_t fx_index;  // current effect
static uint8_t fx_max;    // max # of effects
static uint8_t fx_previndex; //previous effect
static uint8_t ledExitRequest = 0;

// global effects state
uint16_t fx_duration = 0; //effect duration in ms. 0 == persistent
uint32_t fx_starttime = 0; //start time for temporary effect

uint8_t shift = 2;  // start a little bit dimmer

uint32_t bump_amount = 0;
uint8_t bumped = 0;
uint8_t pressure_changed = 0;
uint8_t singletapped = 0;
unsigned int pressurechangedtime = 0;
unsigned int singletaptime = 0;
unsigned int patternChanged = 0;

uint8_t ledsOff = 1;

genome diploid;   // not static so we can access/debug from other files

// some colors
const RgbColor vividViolet = {159, 0, 255};
const RgbColor vividCerulean = {0, 169, 238};
const RgbColor electricGreen = {0, 250, 34};
const RgbColor vividYellow = {255, 227, 2};
const RgbColor vividOrangePeel = {255, 160, 0};
const RgbColor vividRed = {248, 13, 27};
RgbColor vividRainbow[6];

uint8_t effectsStop(void) {
  ledExitRequest = 1;
  return ledsOff;
}

/**
 * @brief   Initialize Led Driver
 * @details Initialize the Led Driver based on parameters.
 *
 * @param[in] leds      length of the LED chain controlled by each pin
 * @param[out] o_fb     initialized frame buffer
 *
 */
void ledStart(uint32_t leds, uint8_t *o_fb, uint32_t ui_leds, uint8_t *o_ui_fb)
{
  unsigned int j;
  led_config.max_pixels = leds;
  led_config.pixel_count = leds;
  led_config.ui_pixels = ui_leds;

  led_config.fb = o_fb;
  led_config.ui_fb = o_ui_fb;

  led_config.final_fb = chHeapAlloc( NULL, sizeof(uint8_t) * led_config.max_pixels * 3 );
  
  for (j = 0; j < leds * 3; j++)
    led_config.fb[j] = 0x0;
  for (j = 0; j < ui_leds * 3; j++)
    led_config.ui_fb[j] = 0x0;

  chSysLock();
  ledUpdate(led_config.fb, led_config.max_pixels);
  chSysUnlock();
}

void uiLedGet(uint8_t index, Color *c) {
  if( index >= led_config.ui_pixels )
    index = led_config.ui_pixels - 1;
  
  c->g = led_config.ui_fb[index*3];
  c->r = led_config.ui_fb[index*3+1];
  c->b = led_config.ui_fb[index*3+2];
}

void uiLedSet(uint8_t index, Color c) {
  if( index >= led_config.ui_pixels )
    index = led_config.ui_pixels - 1;
  
  led_config.ui_fb[index*3] = c.g;
  led_config.ui_fb[index*3+1] = c.r;
  led_config.ui_fb[index*3+2] = c.b;
}

void ledSetRGBClipped(void *fb, uint32_t i,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t shift) {
  if (i >= led_config.pixel_count)
    return;
  ledSetRGB(fb, i, r, g, b, shift);
}

void ledSetRGB(void *ptr, int x, uint8_t r, uint8_t g, uint8_t b, uint8_t shift) {
  uint8_t *buf = ((uint8_t *)ptr) + (3 * x);
  buf[0] = g >> shift;
  buf[1] = r >> shift;
  buf[2] = b >> shift;
}

void ledSetRgbColor(void *ptr, int x, RgbColor c, uint8_t shift) {
  ledSetRGB(ptr, x, c.r, c.g, c.b, shift);
}

void ledSetAllRGB(void *ptr, int n, uint8_t r, uint8_t g, uint8_t b, uint8_t shift) {
  for (int i = 0; i < n; i++) {
    ledSetRGB(ptr, i, r, g, b, shift);
  }
}

void ledSetAllRgbColor(void *ptr, int n, RgbColor c, uint8_t shift) {
  ledSetAllRGB(ptr, n, c.r, c.g, c.b, shift);
}

void ledSetColor(void *ptr, int x, Color c, uint8_t shift) {
  ledSetRGB(ptr, x, c.r, c.g, c.b, shift);
}

Color ledGetColor(void *ptr, int x) {
  Color c;
  uint8_t *buf = ((uint8_t *)ptr) + (3 * x);

  c.g = buf[0];
  c.r = buf[1];
  c.b = buf[2];
  
  return c;
}

void ledSetCount(uint32_t count) {
  if (count > led_config.max_pixels)
    return;
  led_config.pixel_count = count;
}

void setShift(uint8_t s) {
    shift = s;
}

uint8_t getShift(void) {
    return shift;
}

// alpha blend, scale the input color based on a value from 0-255. 255 is full-scale, 0 is black-out.
// uses fixed-point math.
Color alphaPix( Color c, uint8_t alpha ) {
  Color rc;
  uint32_t r, g, b;

  r = c.r * alpha;
  g = c.g * alpha;
  b = c.b * alpha;

  rc.r = (r / 255) & 0xFF;
  rc.g = (g / 255) & 0xFF;
  rc.b = (b / 255) & 0xFF;

  return( rc );  
}

static void do_lightgene(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  uint32_t count = config->count;
  uint32_t loop = config->loop & 0x1FF;
  uint32_t shoot;
  HsvColor hsvC;
  RgbColor rgbC;
  uint32_t i;
  uint32_t tau;
  uint32_t curtime, indextime;
  fix16_t time, space;
  fix16_t twopi;
  fix16_t spacetime;
  uint8_t overrideHSV = 0;
  uint8_t overshift;
  uint32_t hue_rate;
  uint8_t hue_dir;
  uint32_t hue_temp;
  // diploid is static to this function and set when the lightgene is selected

  static uint32_t reftime_lg = 0;
  static uint8_t sat_offset = 0;
  
  tau = (uint32_t) map(diploid.cd_rate, 0, 255, 700, 8000);
  curtime = chVTGetSystemTime();
  if( (curtime - reftime_lg) > tau )
    reftime_lg = curtime;
  indextime = reftime_lg - curtime;

  if( bumped ) {
    bumped = 0;
    sat_offset = satadd_8(sat_offset, map(diploid.accel, 0, 255, 0, 24));
  } else {
    if( (loop % 2) == 0 )  // cheesy make the time constant to baseline longer.
      sat_offset = satsub_8(sat_offset, 1);
  }
  for( i = 0; i < count; i++ ) {
    overrideHSV = 0;
    // compute one pixel's color
    // count is the current pixel index
    // loop is the current point in effect cycle, e.g. all effects loop on a 0-511 basis
    // hue chromosome
    hue_rate = (uint32_t) diploid.hue_ratedir & 0xF;
    hue_dir = (((diploid.hue_ratedir >> 4) & 0xF) > 10) ? 1 : 0;
    /*
      refactor: we want the pattern applied from 0-7 to be inversely applied from 8-15
      0 1 2 3 4 5 6 7  7 6 5 4 3 2 1 0
     */
    // 254L cheesily avoids rounding errors.
    if( !hue_dir ) {
      hue_temp = ((128L / (count / 2)) * i + (loop * hue_rate)) - 0L;
      hue_temp &= 0x1FF;
      if( hue_temp <= 0xFF )
	hsvC.h = (uint8_t) hue_temp;
      else {
	hsvC.h = (uint8_t) (511-hue_temp);
      }
    } else {
      hue_temp = ((128L / (count / 2)) * i - (loop * hue_rate)) - 0L;
      hue_temp &= 0x1FF;
      if( hue_temp <= 0xFF )
	hsvC.h = (uint8_t) hue_temp;
      else {
	hsvC.h = (uint8_t) (511-hue_temp);
      }
    }
    hsvC.h = map_16( (int16_t) hsvC.h, 0, 255,
		     (int16_t) diploid.hue_base, (int16_t) diploid.hue_bound );
    
    // saturation chromosome
    hsvC.s = satadd_8(diploid.sat, sat_offset);

    // compute the value overlay
    // use cos b/c value is 1.0 when input is 0
    // value = 255 * cos( cd_period * 2pi * (i/(count-1))
    //                    +/- (indextime / tau(cd_rate)) * 2pi )
    // sign of rate is determined by cd_dir

    twopi = fix16_mul( fix16_from_int(2), fix16_pi );
    //twopi = 3.14159265359 * 2.0;
        space = fix16_mul(twopi, fix16_mul( fix16_from_int(diploid.cd_period),
                fix16_div(fix16_from_int(i), fix16_from_int(count-1)) ));

    time = fix16_mul(twopi, fix16_div( fix16_from_int(indextime), fix16_from_int(tau) ));
    
    // space +/- time based on direction
    if( diploid.cd_dir > 128 ) {
      spacetime = fix16_add( space, time );
      //spacetime = space + time;
    } else {
      spacetime = fix16_sub( space, time );
      //spacetime = space - time;
    }

    // hsv.v = 127 * (1 + cos(spacetime))
    hsvC.v = (uint8_t) fix16_to_int( fix16_mul( fix16_from_int(127),
						fix16_add( fix16_from_int(1),
							   fix16_cos(spacetime))));

    if( diploid.nonlin > 127 )
      // add some nonlinearity to gamma-correct brightness
      hsvC.v = (uint8_t) (((uint16_t) hsvC.v * (uint16_t) hsvC.v) >> 8 & 0xFF);

    // now compute lin effect, but only if the threshold is met
    if( diploid.lin < 90 ) {  // rare variant after a summing expression ~3% chance
      shoot = loop % count;
      if( shoot == i ) {
	overrideHSV = 1;
      }
    }

    // now compute strobe effect, but only if the threshold is met
    if( diploid.strobe < 10 ) {
      // for now, do nothing...this one is a pain in the ass to implement and probably not too interesting anyways
    }

    // go from RGB to HSV for a particular pixel
    if( !overrideHSV ) {
      rgbC = HsvToRgb(hsvC);
      ledSetRGB(fb, i, rgbC.r, rgbC.g, rgbC.b, shift);
    } else {
      overshift = shift - 2; // make this effect brighter so it's obvious
      if( overshift > 4 )
	overshift = 4;
      ledSetRGB(fb, i, 255, 255, 255, overshift);
    }
  }
}

static void lg0FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg0", lg0FB);

static void lg1FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg1", lg1FB);

static void lg2FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg2", lg2FB);

static void lg3FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg3", lg3FB);

static void lg4FB(struct effects_config *config) {
  do_lightgene(config);
}
orchard_effects("Lg4", lg4FB);

static void strobePatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  
  uint16_t i;
  uint8_t oldshift = shift;
  static uint32_t  nexttime = 0;
  static uint8_t   strobemode = 1;
  
  shift = 0;

  if( strobemode && (chVTGetSystemTime() > nexttime) ) {
    for( i = 0; i < count; i++ ) {
      if( (rand() % (unsigned int) count) < ((unsigned int) count / 3) )
	ledSetRGB(fb, i, 255, 255, 255, shift);
      else
	ledSetRGB(fb, i, 0, 0, 0, shift);
    }

    nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
    strobemode = 0;
  }

  else if( !strobemode && (chVTGetSystemTime() > nexttime) ) {
    ledSetAllRGB(fb, count, 0, 0, 0, shift);
    
    nexttime = chVTGetSystemTime() + 30 + (rand() % 25);
    strobemode = 1;
  }

  shift = oldshift;
}
orchard_effects("strobe", strobePatternFB);


/* Jeanie's effects section */
static void interactivePatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  static uint8_t tapfactor = 1;

  if(pressure_changed){
    pressure_changed = 0;
    chprintf(stream, "%s\n\r", "pressure changed, strobing!");
    effectsSetTempPattern(effectsNameLookup("strobe"), 1000);
  }

  if(bumped){
    bumped = 0;
    chprintf(stream, "%s\n\r", "bumped, strobing!");
    effectsSetTempPattern(effectsNameLookup("strobe"), 1000);
  }

  if(singletapped){
    singletapped = 0;
    tapfactor = tapfactor == 5 ? 1 : tapfactor + 1;
  }

  int currHue = (loop*tapfactor)%255;
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV); 
  ledSetAllRGB(fb, count, (c.r), (c.g), (int)(c.b), shift);
}
orchard_effects("AAAinteractive", interactivePatternFB);

//just a boring blink
static void boringStrobePatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  static int white = 0;

  if(loop % 6 == 0){
    white = white == 0 ? 255 : 0;
    ledSetAllRGB(fb, count, white, white, white, shift);
  }
}
orchard_effects("boringStrobe", boringStrobePatternFB);

static void changeOnDropVividRainbow(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int count = config->count;

	static int currentColor;
	//if there isn't a color already we need to run this to set the color
  ledSetAllRgbColor(fb, count, vividRainbow[currentColor], shift/2);

	if(bumped){
		bumped=0;
		//chprintf(stream, "%s", "New Color: ");
		//chprintf(stream, "%d\n\r", currentColor);
		//update the color by one and set it
		currentColor = (currentColor + 1)%6;
    ledSetAllRgbColor(fb, count, vividRainbow[currentColor], shift/2);
	}
}
orchard_effects("changeOnDropVividRainbow", changeOnDropVividRainbow);

//notes for this effect:
//need to set some reasonable temperature range that will work well for on-playa
//also need to offset the on-board temperature heating.. one option would be to get some default value at boot and subtract
//otherwise could also just figure it out from testing.
static void temperatureTestEffect(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int loop = config->loop;
	int count = config->count;
	//saturation percent for the temp colors (out of 100)
	int satLevel = 80;

	//set some floor/ceiling temperatures.
	const float maxTemp = 40;
	const float minTemp = 20;
	//temp is in milli deg C
	float temp;
	static float persistentTemp;

	if(loop%10==0){
		temp = (float)analogReadTemperature()/(float)1000;
		persistentTemp = temp;
		persistentTemp = persistentTemp < minTemp ? minTemp : persistentTemp;
		persistentTemp = persistentTemp > maxTemp ? maxTemp : persistentTemp;
		//chprintf(stream, "%s", "Current Temp: ");
		//chprintf(stream, "%f\n\r", persistentTemp);
	}

	//then convert to 0-255 for hsv color to make it purdy
	//I'm using float bc I don't like integer rounding.. let's just chop it off afterward.
	float tempHueFloat = ((persistentTemp-minTemp)/((maxTemp-minTemp))*255);
	int tempHue = (int)tempHueFloat;
	int tempSat = 255*satLevel/100;
	HsvColor tempHSV = {tempHue, tempSat, 255};

	//convert back to rgb and set the LED color
	ledSetAllRgbColor(fb, count, HsvToRgb(tempHSV), shift);
}
orchard_effects("temperatureTestEffect", temperatureTestEffect);

extern uint8_t scopemode_g;
static void dbColorChangeAndIntensityEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  // int loop = config->loop; // variable not referenced, comment out to clean up compile
  float level;

  scopemode_g = 2; // this selects db mode
  //let's assume some background and max decibel level 
  int bkgndDB = 30;
  int maxDB = 80;
  //there's no Math.max in C so we have to do something like this to limit the min/max
  //max:
  if(cur_db > maxDB) level = maxDB;
  //min:
  if(cur_db - bkgndDB < 1) level = (float)1;
  else level = (float)(cur_db - bkgndDB);

  level = (level/((float)maxDB-(float)bkgndDB));

  //now let's smooth this puppy out
  //with a very lamely implemented running avg of the last three level readings
  static float avg1;
  static float avg2;
  static float avg3;
  static float sum;
  sum = sum - avg1;
  avg1 = avg2;
  avg2 = avg3;
  avg3 = level;
  sum = sum + avg3;
  float avgLevel = sum/3;
  
  //let's iterate through a rainbow of colors to make it prettier
  //int currHue = loop%255;
  //or, we could also make the hue a function of the current level :o
  //let's do some subtraction to make the top be red
  int currHue = (int)(255-(255*avgLevel));
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV);
  
  ledSetAllRGB(fb, count, (int)(c.r*avgLevel), (int)(c.g*avgLevel), (int)(c.b*avgLevel), (int)(shift*avgLevel));
}
orchard_effects("DBcolor", dbColorChangeAndIntensityEffect);

static void accelEffect(struct effects_config *config) {
  //todo: it does a weird color switch at 0 degrees for some reason, fix this

  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop; 

  struct accel_data data;
  //let's get the xyz coordinates every so often..
  //things seem to get grumpy if you get the values too often :o
  if(loop % 3 == 0){ 
   gyro_Get_X_Axes(&data);
  }


  //let's make an angle from the xy coordinates
  float angle = atan2(data.x, data.y) * (180/3.14159) - 90;
  angle = angle < 0 ? 360 + angle : angle;  // Ensure positive angle
  //convert it into the hue 
	int angleHue = (int)((angle/360)*255);
  //do some slight smoothing:
  static int avg1;
  static int avg2;
  static int avg3;
  static int sum;
  sum = sum - avg1;
  avg1 = avg2;
  avg2 = avg3;
  avg3 = angleHue;
  sum = sum + avg3;
  int avgAngle = sum/3;

	//then convert to 0-255 for hsv color to make it purdy
	HsvColor angleHSV = {avgAngle, 255, 255};

	//convert back to rgb and set the LED color
	ledSetAllRgbColor(fb, count, HsvToRgb(angleHSV), shift);
}
orchard_effects("accel", accelEffect);

static void barometerTestEffect(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;

  static float press;
  press = press == 0 ? baro_pressure : press; 

  static int on = 0;


 if(loop % 10 == 0){
   if (baro_pressure - press > 100){
     chprintf(stream, "%s", "PRESSURE INCREASE!!!");
     on = 1;
   }
   if (baro_pressure - press < -100){
      chprintf(stream, "%s", "PRESSURE DECREASE!!!");
      on = 0;
   }
   //chprintf(stream, "%s", "Current pressure: ");
	 //chprintf(stream, "%f\n\r", baro_pressure);
   //chprintf(stream, "%s", "Prev pressure: ");
	 //chprintf(stream, "%f\n\r", press);
   press = baro_pressure;
 }

  int currHue = loop%255;
  HsvColor currHSV = {currHue, 255, 255};
  RgbColor c = HsvToRgb(currHSV); 
  ledSetAllRGB(fb, count, (c.r*on), (c.g*on), (int)(c.b*on), shift);
}
orchard_effects("barometer", barometerTestEffect);

// Time sync test pattern
static void timesynctest(struct effects_config *config){
	uint8_t *fb = config->hwconfig->fb;
	int count = config->count;
  int loop = config->loop;

  // Each loop value lasts for 35 ms
  // 1 s is 1000/35 ~ 30 steps
  // 6 colors in the rainbow
  ledSetAllRgbColor(fb, count, vividRainbow[(loop / 30) % 6], shift);
}
orchard_effects("timesynctest", timesynctest);

/* end of Jeanie's effects section */

static void testPatternFB(struct effects_config *config) {
  uint8_t *fb = config->hwconfig->fb;
  int count = config->count;
  int loop = config->loop;
  
  int i = 0;

  while (i < count) {
    if (loop & 1) {
      /* Black */
      ledSetRGB(fb, (i++ + loop) % count, 0, 0, 0, shift);

      /* Black */
      ledSetRGB(fb, (i++ + loop) % count, 0, 0, 0, shift);

      /* Red */
      ledSetRGB(fb, (i++ + loop) % count, 128, 0, 0, shift);
    }
    else {
      /* Red */
      ledSetRGB(fb, (i++ + loop) % count, 128, 0, 0, shift);

      /* Black */
      ledSetRGB(fb, (i++ + loop) % count, 0, 0, 0, shift);

      /* Black */
      ledSetRGB(fb, (i++ + loop) % count, 0, 0, 0, shift);
    }
  }
 
}
orchard_effects("safetyPattern", testPatternFB);



#define BUMP_DEBOUNCE 300 // 300ms debounce to next bump

void bump(uint32_t amount) {
  static unsigned int bumptime = 0;
  
  bump_amount = amount;
  if( chVTGetSystemTime() - bumptime > BUMP_DEBOUNCE ) {
    bumptime = chVTGetSystemTime();
    bumped = 1;
  }
}

static void draw_pattern(void) {
  const OrchardEffects *curfx;
  
  curfx = orchard_effects_start();
  
  fx_config.loop++;

  if( bump_amount != 0 ) {
    fx_config.loop += bump_amount;
    bump_amount = 0;
  }

  curfx += fx_index;

  curfx->computeEffect(&fx_config);
}

const char *effectsCurName(void) {
  const OrchardEffects *curfx;
  curfx = orchard_effects_start();
  curfx += fx_index;
  
  return (const char *) curfx->name;
}

uint8_t effectsNameLookup(const char *name) {
  uint8_t i;
  const OrchardEffects *curfx;

  curfx = orchard_effects_start();
  if( name == NULL ) {
    return 0;
  }
  
  for( i = 0; i < fx_max; i++ ) {
    if( strcmp(name, curfx->name) == 0 ) {
      return i;
    }
    curfx++;
  }
  
  return 0;  // name not found returns default effect
}

// checks to see if the current effect is one of the lightgenes
// if it is, updates the diploid genome to the current effect
void check_lightgene_hack(void) {
  const struct genes *family;
  uint8_t family_member = 0;
  
  if( strncmp(effectsCurName(), "Lg", 2) == 0 ) {
    family = (const struct genes *) storageGetData(GENE_BLOCK);
    // handle lightgene special case
    family_member = effectsCurName()[2] - '0';
    computeGeneExpression(&(family->haploidM[family_member]),
			  &(family->haploidP[family_member]), &diploid);
  }
}

const char *lightgeneName(void) {
  return diploid.name;
}

void effectsSetPattern(uint8_t index) {
  if(index > fx_max) {
    fx_index = 0;
    return;
  }

  fx_index = index;
  patternChanged = 1;
  check_lightgene_hack();
}

uint8_t effectsGetPattern(void) {
  return fx_index;
}

void effectsNextPattern(int skipstrobe) {
  fx_index = (fx_index + 1) % fx_max;

  if(skipstrobe) {
    if(strncmp(effectsCurName(), "strobe", 6) == 0) {
      fx_index = (fx_index + 1) % fx_max;
    }
  }

  patternChanged = 1;
  check_lightgene_hack();
}

void effectsPrevPattern(int skipstrobe) {
  if( fx_index == 0 ) {
    fx_index = fx_max - 1;
  } else {
    fx_index--;
  }

  if(skipstrobe) {
    if(strncmp(effectsCurName(), "strobe", 6) == 0) {
      if( fx_index == 0 ) {
	fx_index = fx_max - 1;
      } else {
	fx_index--;
      }
    }
  }
  
  patternChanged = 1;
  check_lightgene_hack();
}

static void blendFbs(void) {
  uint8_t i;
  // UI FB + effects FB blend (just do a saturating add)
  for( i = 0; i < led_config.ui_pixels * 3; i ++ ) {
    led_config.final_fb[i] = satadd_8(led_config.fb[i], led_config.ui_fb[i]);
  }

  // copy over the remainder of the effects FB that extends beyond UI FB
  for( i = led_config.ui_pixels * 3; i < led_config.max_pixels * 3; i++ ) {
    led_config.final_fb[i] = led_config.fb[i];
  }
  if( ledExitRequest ) {
    for( i = 0; i < led_config.max_pixels * 3; i++ ) {
      led_config.final_fb[i] = 0; // turn all the LEDs off
    }
  }
}

int32_t time_slop = 230;
static THD_FUNCTION(effects_thread, arg) {

  (void)arg;
  chRegSetThreadName("effects");
  uint32_t last_time = getNetworkTimeMs();
  int32_t offset;

  while (!ledsOff) {

    // handle the case of e.g. negative clock drift or big step adjustment
    // in negative time direction
    offset = getNetworkTimeMs() - last_time;
    offset = offset < 0 ? -offset : offset;
    if( offset > 2 * EFFECTS_REDRAW_MS )
      last_time = getNetworkTimeMs();
    
    if( (getNetworkTimeMs() - last_time) > EFFECTS_REDRAW_MS ) {
      last_time += EFFECTS_REDRAW_MS;

      blendFbs();

      // transmit the actual framebuffer to the LED chain
      chSysLock();
      ledUpdate(led_config.final_fb, led_config.pixel_count);
      chSysUnlock();

      // wait until the next update cycle
      chThdYield();
      chThdSleepMilliseconds(1);
      
      // re-render the internal framebuffer animations
      draw_pattern();
    } else {
      chThdSleepMilliseconds(1);
    }

    if( ledExitRequest ) {
      // force one full cycle through an update on request to force LEDs off
      blendFbs();
      chSysLock();
      ledUpdate(led_config.final_fb, led_config.pixel_count);
      ledsOff = 1;
      chSysUnlock();
      // chThdExitS(MSG_OK);
      return; // this is the same as an exit
    }
  }
  return;
}

void listEffects(void) {
  const OrchardEffects *curfx;

  curfx = orchard_effects_start();
  chprintf(stream, "max effects %d\n\r", fx_max );

  while( curfx->name ) {
    chprintf(stream, "%s\n\r", curfx->name );
    curfx++;
  }
}

void effectsStart(void) {
  const OrchardEffects *curfx;
  
  fx_config.hwconfig = &led_config;
  fx_config.count = led_config.pixel_count;
  fx_config.loop = getNetworkTimeMs() / EFFECTS_REDRAW_MS;

  vividRainbow[0] = vividRed;
  vividRainbow[1] = vividOrangePeel;
  vividRainbow[2] = vividYellow;
  vividRainbow[3] = electricGreen;
  vividRainbow[4] = vividCerulean;
  vividRainbow[5] = vividViolet;
  
  strncpy( diploid.name, "err!", GENE_NAMELENGTH ); // in case someone references before init

  curfx = orchard_effects_start();
  fx_max = 0;
  fx_index = 0;
  while( curfx->name ) {
    fx_max++;
    curfx++;
  }
  check_lightgene_hack();

  draw_pattern();
  ledExitRequest = 0;
  ledsOff = 0;

  //  chThdCreateStatic(waEffectsThread, sizeof(waEffectsThread),
  //      NORMALPRIO - 6, effects_thread, &led_config);
  chThdCreateFromHeap(NULL, THD_WORKING_AREA_SIZE(256), "effects", NORMALPRIO - 6, effects_thread, &led_config);
}

OrchardTestResult test_led(const char *my_name, OrchardTestType test_type) {
  (void) my_name;
  
  OrchardTestResult result = orchardResultPass;
  uint16_t i;
  uint8_t interactive = 0;
  
  switch(test_type) {
  case orchardTestPoweron:
    // the LED is not easily testable as it's "write-only"
    return orchardResultUnsure;
  case orchardTestInteractive:
    interactive = 20;  // 20 seconds to evaluate LED state...should be plenty
  case orchardTestTrivial:
  case orchardTestComprehensive:
    orchardTestPrompt("Preparing", "LED test", 0);
    while(ledsOff == 0) {
      effectsStop();
      chThdYield();
      chThdSleepMilliseconds(100);
    }

    // green pattern
    for( i = 0; i < led_config.pixel_count * 3; i += 3 ) {
      led_config.final_fb[i] = 255;
      led_config.final_fb[i+1] = 0;
      led_config.final_fb[i+2] = 0;
    }
    chSysLock();
    ledUpdate(led_config.final_fb, led_config.pixel_count);
    chSysUnlock();
    orchardTestPrompt("green LED test", "", 0);
    orchardTestPrompt("press button", "to advance", interactive);
    chThdSleepMilliseconds(200);

    // red pattern
    for( i = 0; i < led_config.pixel_count * 3; i += 3 ) {
      led_config.final_fb[i] = 0;
      led_config.final_fb[i+1] = 255;
      led_config.final_fb[i+2] = 0;
    }
    chSysLock();
    ledUpdate(led_config.final_fb, led_config.pixel_count);
    chSysUnlock();
    chThdSleepMilliseconds(200);
    orchardTestPrompt("red LED test", "", 0);
    orchardTestPrompt("press button", "to advance", interactive);
    chThdSleepMilliseconds(200);

    // blue pattern
    for( i = 0; i < led_config.pixel_count * 3; i += 3 ) {
      led_config.final_fb[i] = 0;
      led_config.final_fb[i+1] = 0;
      led_config.final_fb[i+2] = 255;
    }
    orchardTestPrompt("blue LED test", "", 0);
    chSysLock();
    ledUpdate(led_config.final_fb, led_config.pixel_count);
    chSysUnlock();
    chThdSleepMilliseconds(200);
    orchardTestPrompt("press button", "to advance", interactive);
    chThdSleepMilliseconds(200);

    orchardTestPrompt("LED test", "finished", 0);
    // resume effects
    effectsStart();
    return result;
  default:
    return orchardResultNoTest;
  }
  
  return orchardResultNoTest;
}
orchard_test("ws2812b", test_led);
