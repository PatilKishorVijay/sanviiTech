// (c) 2010 Johnson Controls.  All Rights reserved.

#ifndef _ECRECOVERYHANDLER_H_
#define _ECRECOVERYHANDLER_H_

#include "evbushandler.h"
#include "utilTimestamp.h"
#include "eventpersist.h"

using namespace eventchannel;
using namespace videoedge::util;

/**
 * The message handler for the Storage Manager<br>
 * This handler translates event channel messages into a form that can me emailed. The email is sent.
 */

class EcrecoveryHandler final : public EctoolMessageHandlerBase
{
public:
    EcrecoveryHandler() : EctoolMessageHandlerBase() {}
    ~EcrecoveryHandler();

    /** Preprocessing step<br>
     * This method is called prior to starting the DBUS subscriber loop.<br>
     * The connection to the EventCacheDatabaseManager is established here.
     * read from disk.
     */
    bool DoPreprocess();

    /** Process incoming DBUS messages<br>
     * Event Cache Database control messages are handled here.
     */
    bool ProcessMessage(EventChannelMessage *pMsg) { return true; }

    /**
     * Handle idle time between FDMon fd processing.<br>
     * The transmission of messages is to the site manager are handled here.
     * @return none
     */
    void ProcessIdleTime();

private:
    /**
     * Teminate the transmission process.<br>
     * Sends the end of data marker to the clinet.<br>
     * Sets the global loop terminate flag to "true"<br>
     */
    void TerminateTransmission();

    /** The EventCacheDatabaseManager interface */
    EventCacheDatabaseManager *m_DB = nullptr;

    /** The max number of records to retrieve */
    sqlite3_int64 m_numRecords = 0;

    /** The current incremental start sequence number */
    sqlite3_int64 m_iStart = 0;

    /** The current incremental end sequence number */
    sqlite3_int64 m_iEnd = 0;

    /** Flag: A query is in progress */
    bool m_fQueryInProgress = false;

    /** The read block size in sequence numbers */
    static const sqlite3_int64 m_BlockSize = 1000;

    /** The number of records actually sent */
    sqlite3_int64 m_numRecordsSent = 0;

    /** Recovery start time */
    time_t m_start_time = 0;
};

#endif
