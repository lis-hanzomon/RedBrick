
/* https://gist.github.com/takamin/7cc9a245536510c630e3#file-daemonize-c */

/*!
 * create a typical daemon process.
 *
 * @param [in] pidfilepath      The pid file pathname.
 *                              If NULL, it will never record.
 * @param [in] syslog_ident     syslog ident.
 * @param [in] syslog_option    syslog option.
 * @param [in] syslog_facility  syslog facility.
 * @return this function        1:a daemon created, 0: error
 */
int daemonize(
  const char* pidfilepath,
  const char* syslog_ident,
  int syslog_option,
  int syslog_facility);
