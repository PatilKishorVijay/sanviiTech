///////////////////////////////////////////////////////////////////////////////////////
///
/// (c) 2010 Tyco International Ltd. and its respective companies. All rights reserved.
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef _ECRECOVERY_H_
#define _ECRECOVERY_H_

/************************************************************/
// ecrecovery limits and definitions
/************************************************************/


/** Report an Evbus error to std::cerr
 * @param msg Ptr. to the nul terminated error prefix text.
 */
void reportEvbusError(const char *msg = 0);

/** Event Handler Callback */
void handleIncomingEvent(const char *source, EVBusEvent *event);

/** Alarm Handler Callback */
void handleIncomingAlarm(const char *source, EvbusSeverity severity, EVBusEvent *event);

/** Set a watch on an alarm<br>
 * If this call fails, then the error is reported and ectool exits after closing the DBUS connection.
 * @param message Ptr. to nul terminated message name.
 * @returns none
 */
void setupAlarmWatch(const char *message);

/** Set a watch on an event<br>
 * If this call fails, then the error is reported and ectool exits after closing the DBUS connection.
 * @param message Ptr. to nul terminated message name.
 * @returns none
 */
void setupEventWatch(const char *message);

/** Setup RunMode specific filters */
void setupRunModeFilters();

/** Send an event/alarm message and report errors
 * @param message Ptr. to the EVBusEvent to be sent
 * @param fKeepGoing Flag: If an error is detected then log it and keep going.
 * @return bool Returns true if successful. Returns false on failure.
 */
bool sendEvbusMessage(EVBusEvent *message, bool fKeepGoing);

/** Log a debug message<br>
 * This code only executes if gParams->ExtraLogging() returns true.
 * @returns none.
 */
#define logDebug(format, args...) \
    { \
        if (gParams.ExtraLogging()) syslog(LOG_INFO, "[%s] " format "", __FUNCTION__, ##args); \
    }

#endif
