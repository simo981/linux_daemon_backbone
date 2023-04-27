#include "main.h"
#define TARGET_NAME "DAEMON"
#define SLEEP_TIME_SECOND 60
#define FALSE 0
#define TRUE 1

static volatile sig_atomic_t EXIT = FALSE;
static volatile sig_atomic_t CONFR = FALSE;

void sigterm_handler(int code) { EXIT = TRUE; }

void sighup_handler(int code) { CONFR = TRUE; }

int main(void) {
  pid_t daemon_pid;
  SYSEQV("PID_FORK", daemon_pid, fork(), -1);
  IFG(daemon_pid, 0, exit(EXIT_SUCCESS));
  SYSEQ("SETSID", setsid(), -1);
  sigset_t *mask = NULL;
  SYSEQV("MALLOC", mask, (sigset_t *)malloc(sizeof(sigset_t)), NULL);
  sigfillset(mask);
  struct sigaction *handler = NULL;
  SYSEQV("CALLOC", handler,
         (struct sigaction *)calloc(1, sizeof(struct sigaction)), NULL);
  SYSNEQ("MASKING_SIGNALS", pthread_sigmask(SIG_SETMASK, mask, NULL), 0);
  handler->sa_handler = sigterm_handler;
  SYSEQ("SETSIGACTION_TERM", sigaction(SIGTERM, handler, NULL), -1);
  handler->sa_handler = sighup_handler;
  SYSEQ("SETSIGACTION_HUP", sigaction(SIGHUP, handler, NULL), -1);
  sigemptyset(mask);
  SYSNEQ("DE_MASKING_SIGNALS", pthread_sigmask(SIG_SETMASK, mask, NULL), 0);
  free(mask);
  free(handler);
  SYSEQV("PID_FORK", daemon_pid, fork(), -1);
  IFG(daemon_pid, 0, exit(EXIT_SUCCESS));
  umask(0022);
  SYSEQ("CHDIR", chdir("/tmp"), -1);
  SYSEQ("CLOSE_UNUSED_FD", close_range(0, 2, CLOSE_RANGE_UNSHARE), -1);
  openlog(TARGET_NAME, LOG_CONS | LOG_PID, LOG_DAEMON);
  syslog(LOG_NOTICE, "DAEMON_STARTED_OK");
  struct timespec wait = {SLEEP_TIME_SECOND, 0};
  sig_atomic_t sleep_return;
  for (;;) {
    sleep_return = nanosleep(&wait, &wait);
    if (sleep_return != 0) {
      if (EXIT) {
        break;
      } else if (CONFR) {
        syslog(LOG_NOTICE, "RELOAD_CONF");
        // RELOAD CONF
        CONFR = FALSE;
      }
    }
  }
  syslog(LOG_NOTICE, "DAEMON_FINISHING_OK");
  closelog();
  exit(EXIT_SUCCESS);
}
