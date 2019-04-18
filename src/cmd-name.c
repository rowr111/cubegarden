#include "hal.h"
#include "shell.h"
#include "chprintf.h"

#include "storage.h"
#include "genes.h"
#include <string.h>

#define NL SHELL_NEWLINE_STR
#define TEXTENTRY_MAXLEN  19  // maximum length of any entered text, not including null char

static char myname[TEXTENTRY_MAXLEN + 1];


void cmd_name(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  const struct genes *family;
  struct genes newFamily;

  
  if (argc <= 0) {
    chprintf(chp, "Usage: name [newname]:"SHELL_NEWLINE_STR);
    return;
  }

  if (strlen(argv[0]) < 2 ) {
    chprintf(chp, "Name to short. Aborting."NL);
    return;
  }
  strncpy(myname, argv[0], TEXTENTRY_MAXLEN);
  chprintf(chp, "Setting name to %s."NL, myname);

  family = (const struct genes *) storageGetData(GENE_BLOCK);
  memcpy(&newFamily, family, sizeof(struct genes));
  strncpy(newFamily.name, myname, GENE_NAMELENGTH);
  newFamily.name[GENE_NAMELENGTH - 1] = '\0'; // enforce the null terminator

  storagePatchData(GENE_BLOCK, (uint32_t *) &newFamily, GENE_OFFSET, sizeof(struct genes));

  chprintf(chp, "Done."NL);
  
  return;
}
