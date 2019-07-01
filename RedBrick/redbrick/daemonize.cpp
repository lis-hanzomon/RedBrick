
/* https://gist.github.com/takamin/7cc9a245536510c630e3#file-daemonize-c */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <syslog.h>

static int daemonized = 0; /* daemonized flag */

int daemonize(
  const char* pidfilepath,
  const char* syslog_ident,
  int syslog_option,
  int syslog_facility)
{
  daemonized = 0; /* initialize */
  if (daemon(0, 0) != -1) {

    /* success to daemonize. */
    daemonized = 1;
	  
    /* open syslog */
    if(syslog_ident) {
      openlog(syslog_ident, syslog_option, syslog_facility);
    }

    /* write daemon pid to the file */
    if(pidfilepath) {
      FILE* pidfile = fopen(pidfilepath, "w+");
      if (pidfile) {
        int pid = getpid();
        fprintf(pidfile, "%d\n", pid);
        fclose(pidfile);
      } else {
        syslog(LOG_ERR,
          "daemonize() : failed to write pid.\n");
      }
    }
  }
  return daemonized;
}
