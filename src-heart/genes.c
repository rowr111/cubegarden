#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "orchard.h"

#include "storage.h"
#include "genes.h"
#include "orchard-math.h"
#include "orchard-ui.h"
#include "touch.h"
#include "shellcfg.h"

#include <string.h>
#include <stdlib.h>

extern genome diploid;

void generateName(char *result) {
  strlen(strcpy(result, "cubemaster"));
}


void computeGeneExpression(const genome *hapM, const genome *hapP,
			   genome *expr) {

  expr->cd_period = 6 - satadd_8_limit(hapM->cd_period, hapP->cd_period, 6);
  expr->cd_rate = (uint8_t) (((uint16_t) hapM->cd_rate + (uint16_t) hapP->cd_rate) / 2);
  expr->cd_dir = satadd_8(hapM->cd_dir, hapP->cd_dir);
  expr->sat = satadd_8(hapM->sat, hapP->sat);
  //rate
  expr->hue_ratedir = 14 - satadd_8_limit(hapM->hue_ratedir & 0xF, hapP->hue_ratedir & 0xF, 14);
  expr->hue_ratedir = (2 + expr->hue_ratedir) % 14;
  //direction
  expr->hue_ratedir |= (satadd_8_limit( (hapM->hue_ratedir >> 4) & 0xF,
					(hapP->hue_ratedir >> 4) & 0xF, 15) << 4);
  expr->hue_base = satsub_8(hapM->hue_base, hapP->hue_base);
  expr->hue_bound = 255 - satsub_8(hapM->hue_bound, hapP->hue_bound);
  expr->lin = satadd_8(hapM->lin, hapP->lin);
  expr->strobe = satadd_8(hapM->strobe, hapP->strobe);
  expr->accel = (uint8_t) (((uint16_t)hapM->accel + (uint16_t)hapP->accel) / 2);
  expr->nonlin = (uint8_t) (((uint16_t) hapM->nonlin + (uint16_t) hapP->nonlin) / 2); // avg it
  // names come from the maternal side in this society
  strncpy(expr->name, hapM->name, GENE_NAMELENGTH);
}

static void generate_gene(struct genome *haploid) {
  char genName[GENE_NAMELENGTH];

  haploid->cd_period = 0;
  haploid->cd_rate = 15;
  haploid->cd_dir = 0;
  haploid->sat = 0x60;
  haploid->hue_base = 0x0;
  haploid->hue_ratedir = 0x0;
  haploid->hue_bound = 255;
  haploid->lin = 0;
  haploid->strobe = 0;
  haploid->accel = 255;
  haploid->nonlin = 255;
  
  generateName(genName);
  strncpy(haploid->name, genName, GENE_NAMELENGTH);
}

#if 0
void cmd_testmap(BaseSequentialStream *chp, int argc, char *argv[]) {
  int16_t i;

  for( i = 0; i < 16; i++ ) {
    chprintf( chp, "%d : %d\n\r", i, map(i, 0, 15, 0, 255) );
  }
  for( i = 0; i < 16; i++ ) {
    chprintf( chp, "%d : %d\n\r", i, map(i, 0, 15, 0, 5) );
  }
}
orchard_shell("testmap", cmd_testmap);
#endif

void print_haploid(BaseSequentialStream *chp, const genome *haploid) {
  chprintf(chp, "Individual %s:\n\r", haploid->name );
  chprintf(chp, " %3d cd_period\n\r", haploid->cd_period );
  chprintf(chp, " %3d cd_rate = %d\n\r", haploid->cd_rate,
	   map(haploid->cd_rate, 0, 255, 700, 8000));
  chprintf(chp, " %3d cd_dir\n\r", haploid->cd_dir );
  chprintf(chp, " %3d sat\n\r", haploid->sat );
  chprintf(chp, " %3d hue_base\n\r", haploid->hue_base );
  chprintf(chp, " %3d hue_ratedir = %d/%d\n\r", haploid->hue_ratedir,
	   haploid->hue_ratedir >> 4 & 0xF, haploid->hue_ratedir & 0xF );
  chprintf(chp, " %3d hue_bound\n\r", haploid->hue_bound );
  chprintf(chp, " %3d lin\n\r", haploid->lin );
  chprintf(chp, " %3d strobe\n\r", haploid->strobe );
  chprintf(chp, " %3d accel\n\r", haploid->accel );
  chprintf(chp, " %3d nonlin\n\r", haploid->nonlin );
}

void cmd_genetweak(BaseSequentialStream *chp, int argc, char *argv[]) {
  int8_t index, value;
  
  if( argc != 2 ) {
    chprintf(chp, "Usage: gtweak <index> <value>\n\r");
    return;
  }
  index = (uint8_t) strtoul(argv[0], NULL, 0);
  value = (uint8_t) strtoul(argv[1], NULL, 0);

  ((char *)&diploid)[index] = value;

  chprintf( chp, "Revised structure.\n\r" );
  print_haploid(chp, &diploid);
}
orchard_shell("gtweak", cmd_genetweak);

void cmd_geneseq(BaseSequentialStream *chp, int argc, char *argv[]) {
  uint8_t which;
  const struct genes *family;
  struct genome diploid;

  family = (const struct genes *) storageGetData(GENE_BLOCK);
  
  if( argc != 1 ) {
    chprintf(chp, "Usage: geneseq <individual>, where <individual> is 0-%d\n\r", GENE_FAMILYSIZE - 1);
    return;
  }
  which = (uint8_t) strtoul( argv[0], NULL, 0 );
  if( which >= GENE_FAMILYSIZE ) {
    chprintf(chp, "Usage: geneseq <individual>, where <individual> is 0-%d\n\r", GENE_FAMILYSIZE - 1);
    return;
  }

  if( family->signature != GENE_SIGNATURE ) {
    chprintf(chp, "Invalid genome signature\n\r" );
    return;
  }
  if( family->version != GENE_VERSION ) {
    chprintf(chp, "Invalid genome version\n\r" );
    return;
  }
  chprintf(chp, "--Maternal Haploid--\n\r");
  print_haploid(chp, &(family->haploidM[which]));
  chprintf(chp, "--Paternal Haploid--\n\r");
  print_haploid(chp, &(family->haploidP[which]));

  chprintf(chp, "--Diploid expression--\n\r");
  computeGeneExpression(&(family->haploidM[which]), &(family->haploidP[which]), &diploid);
  print_haploid(chp, (const genome *) &diploid);
}
orchard_shell("geneseq", cmd_geneseq);

static void init_genes(uint32_t block) {
  struct genes family;
  char genName[GENE_NAMELENGTH];
  int i;

  family.signature = GENE_SIGNATURE;
  family.version = GENE_VERSION;
    
  generateName(genName);
  strncpy(family.name, genName, GENE_NAMELENGTH);

  for( i = 0; i < GENE_FAMILYSIZE; i++ ) {
    generate_gene(&family.haploidM[i]);
    generate_gene(&family.haploidP[i]);
  }

  storagePatchData(block, (uint32_t *) &family, GENE_OFFSET, sizeof(struct genes));
}

int geneStart() {
  const struct genes *family;
  int had_to_init = 0;

  family = (const struct genes *) storageGetData(GENE_BLOCK);

  if( family->signature != GENE_SIGNATURE ) {
    init_genes(GENE_BLOCK);
    had_to_init = 1;
  } else if( family->version != GENE_VERSION ) {
    init_genes(GENE_BLOCK);
    had_to_init = 1;
  }

  return had_to_init;
}

uint8_t getConsent(char *who) {
  font_t font;
  coord_t width;
  coord_t fontheight;
  uint32_t starttime;
  uint32_t curtime, updatetime;
  int8_t interaction_delay = 10;
  uint8_t result = 0;
  uint8_t countdown;
  uint32_t val;
  char timer[16];

  val = captouchRead();
  
  orchardGfxStart();
  font = gdispOpenFont("fixed_5x8");
  width = gdispGetWidth();
  fontheight = gdispGetFontMetric(font, fontHeight);

  gdispClear(Black);
  gdispDrawStringBox(0, fontheight * 2, width, fontheight,
                     "Press any button", font, White, justifyCenter);
  gdispDrawStringBox(0, fontheight * 3, width, fontheight,
                     "to have sex with", font, White, justifyCenter);
  
  gdispDrawStringBox(0, fontheight * 4, width, fontheight,
                     who, font, White, justifyCenter);


  countdown = (uint8_t) abs(interaction_delay);
  result = 0;
  starttime = chVTGetSystemTime();
  updatetime = starttime + 1000;
  while(1) {
    curtime = chVTGetSystemTime();
    if ((val != captouchRead())) {
      result = 1;
      break;
    }
    if ((curtime - starttime) > ((uint32_t) abs(interaction_delay) * 1000)) {
      result = 0;
      break;
    }

    if (curtime > updatetime) {
      chsnprintf(timer, sizeof(timer), "%d", countdown);
      gdispFillArea(0, fontheight * 5, width, fontheight, Black);
      
      gdispDrawStringBox(0, fontheight * 5, width, fontheight,
			 timer, font, White, justifyCenter);
      gdispFlush();
      countdown--;
      updatetime += 1000;
    }
    chThdYield();
    chThdSleepMilliseconds(30);
  }

  gdispFlush();
  gdispCloseFont(font);
  orchardGfxEnd();

  return result;
}
