#include "hal.h"
#include "shell.h"
#include "chprintf.h"
#include "shellcfg.h"

#include "cmd-forward.h"
#include <string.h>
#include "orchard-app.h"
#include "paging.h"
#include "radio.h"

static char *parse_arguments(char *str, char **saveptr);
static bool cmdexec(const ShellCommand *scp, BaseSequentialStream *chp,
	     char *name, int argc, char *argv[]);

void cmd_forward(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argc;

  if (argc == 0) {
    chprintf(chp, "Usage: forward \"<command>\""SHELL_NEWLINE_STR);
    chprintf(chp, "Runs the given command on nearby devices"SHELL_NEWLINE_STR);
    return;
  }

  radioAcquire(radioDriver);
  radioSend(radioDriver, RADIO_BROADCAST_ADDRESS, radio_prot_forward, strlen(argv[0]) + 1, argv[0]);
  radioRelease(radioDriver);
  
  return;
}

orchard_shell("forward", cmd_forward);

void handleRadioForward(uint8_t prot, uint8_t src, uint8_t dst, uint8_t length, const void *data) {
  (void)prot;
  (void)src;
  (void)dst;
  (void)length;

  int n;
  char *lp, *cmd, *tokp;
  BaseSequentialStream *chp = shell_cfg.sc_channel;
  const ShellCommand *scp = shell_cfg.sc_commands;
  char *args[SHELL_MAX_ARGUMENTS + 1];

  lp = parse_arguments((char *)data, &tokp);
  cmd = lp;
  n = 0;

  while ((lp = parse_arguments(NULL, &tokp)) != NULL) {
    args[n++] = lp;
  }
  args[n] = NULL;

  if (cmd != NULL) {
    cmdexec(scp, chp, cmd, n, args);
  }
}

static char *parse_arguments(char *str, char **saveptr) {
  char *p;

  if (str != NULL)
    *saveptr = str;

  p = *saveptr;
  if (!p) {
    return NULL;
  }

  /* Skipping white space.*/
  p += strspn(p, " \t");

  if (*p == '"') {
    /* If an argument starts with a double quote then its delimiter is another
       quote.*/
    p++;
    *saveptr = strpbrk(p, "\"");
  }
  else {
    /* The delimiter is white space.*/
    *saveptr = strpbrk(p, " \t");
  }

  /* Replacing the delimiter with a zero.*/
  if (*saveptr != NULL) {
    *(*saveptr)++ = '\0';
  }

  return *p != '\0' ? p : NULL;
}

static bool cmdexec(const ShellCommand *scp, BaseSequentialStream *chp,
                      char *name, int argc, char *argv[]) {

  while (scp->sc_name != NULL) {
    if (strcmp(scp->sc_name, name) == 0) {
      scp->sc_function(chp, argc, argv);
      return false;
    }
    scp++;
  }
  return true;
}
