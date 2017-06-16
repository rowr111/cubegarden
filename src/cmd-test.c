#include "hal.h"
#include "ch.h"
#include "shell.h"
#include "chprintf.h"

#include "orchard-test.h"
#include "test-audit.h"

#include "orchard-app.h"

int start_test = 0;
int test_started = 0;

void testCommand(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  
  const TestRoutine *test;
  OrchardTestResult  test_result;
  OrchardTestType  test_type;
  const OrchardApp *test_app;
  
  if( argc != 2 ) {
    chprintf(chp, "Usage: test <testname> <testtype>, where testname is one of:\n\r");
    orchardListTests(chp);
    chprintf(chp, "And testtype is according to the following table:\n\r" );
    chprintf(chp, "  0 - Power On\n\r" );
    chprintf(chp, "  1 - Trivial\n\r" );
    chprintf(chp, "  2 - Comprehensive\n\r" );
    chprintf(chp, "  3 - Interactive\n\r" );
    return;
  }

  test_app = orchardAppByName("~testmode");
  if (test_app) {
    orchardAppRun(test_app);
    // give some time for the app to launch
    chThdYield();
    chThdSleepMilliseconds(500);
  }
  
  test = orchardGetTestByName(argv[0]);
  test_type = (OrchardTestType) strtoul(argv[1], NULL, 0);

  if (test == NULL) {
    chprintf(chp, "Test %s was not found.\n\r", argv[0]);
    return;
  }

  test_result = orchardTestRun(test, test_type);

  chprintf(chp, "Test result code is %d\n\r", (int8_t) test_result);

  return;
}

void cmd_printaudit(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void) chp;
  (void) argc;
  (void) argv;
  
  auditPrintLog(chp);
}

void cmd_auditcheck(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void) chp;
  OrchardTestType  test_type;

  if( argc != 1 ) {
    chprintf(chp, "Usage: auditcheck <testtype>\n\r");
    return;
  }
  test_type = (OrchardTestType) strtoul(argv[0], NULL, 0);
  chprintf(chp, "audit check result: %x\n\r", auditCheck(test_type));
}

void cmd_testall(BaseSequentialStream *chp, int argc, char *argv[])
{
  (void) chp;
  OrchardTestType  test_type;
  const OrchardApp *test_app;

  test_app = orchardAppByName("~testmode");
  if( test_app ) {
    start_test = 1;
    // give some time for the app to launch
    while( !test_started ) {
      chThdYield();
      chThdSleepMilliseconds(100);
    }
  }
  
  if( argc != 1 ) {
    chprintf(chp, "Usage: testall <testtype>\n\r");
    return;
  }
  test_type = (OrchardTestType) strtoul(argv[0], NULL, 0);

  orchardTestRunAll(chp, test_type);

  start_test = 0;
  test_started = 0;
}
