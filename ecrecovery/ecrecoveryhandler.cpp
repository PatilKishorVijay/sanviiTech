// (c) 2010 Johnson Controls.  All Rights reserved.

#include "ecrecoveryhandler.h"
#include "ecrecoveryparams.h"
#include "eventcache.h"
#include "failover.h"
#include "failovernvrid.h"
#include "failoverNvrState.h"
#include "utilSystem.h"
#include "evbus_fdmon.h"

#include <mediadatabase/mediadatabase.h>
#include <mediadatabase/failovernvr.h>
#include <mediadatabase/postgresqlhandler.h>

using namespace sqliteutil;
using namespace eventchannel;
using namespace videoedge::util;
using namespace videoedge::failover;
namespace ADMEDIADB = videoedge::mediadatabase;

extern EcrecoveryParameters gParams;
extern CFileDescriptorEventLoop gEventLoop;

EcrecoveryHandler::~EcrecoveryHandler()
{
    if (m_DB)
    {
        delete m_DB;
        m_DB = 0;
    }
}

bool EcrecoveryHandler::DoPreprocess()
{
    if (!m_DB)
    {
        std::string dbPath;

        if (gParams.m_fDatabase)
        {
            // Command line override of database path
            dbPath = gParams.m_strDatabase;
        }
        else if (
            gParams.m_failoverState == FAILOVER_STATE_SECONDARY_ACTIVE
            || gParams.m_failoverState == FAILOVER_STATE_SECONDARY_MONITORING)
        {
            if (gParams.m_nvrid.length())
            {
                bool myStaticSecondaryIPOrName = false;
                std::string nvridAddress;
                std::string nvridName;
                const std::string sNvrIdPrefix = videoedge::failover::CNvrId::nvrIdToPrefix(gParams.m_nvrid);
                if (sNvrIdPrefix == NvrIdVirtualIpPrefix)
                {
                    // Create an address without the leading "IP" prefix
                    // Check to see if the nvrid targets the secondary instead of a primary nvr.
                    nvridAddress = gParams.m_nvrid.substr(2);
                    myStaticSecondaryIPOrName = isMyNetworkInterfaceIpAddress(nvridAddress);
                }
                else if (sNvrIdPrefix == NvrIdVirtualNamePrefix)
                {
                    // Check if this is my hostname
                    std::string sVirtualFQDN = gParams.m_nvrid.substr(2);
                    // It could be an FQDN, so convert to hostname.
                    nvridName = CFailoverNvrUnitState::fqdnNameToLocalName(sVirtualFQDN);
                    const std::string myHostname = videoedge::util::getServerHostName();
                    if (myHostname == nvridName)
                    {
                        myStaticSecondaryIPOrName = true;
                    }
                }

                if (myStaticSecondaryIPOrName)
                {
                    syslog(
                        LOG_INFO,
                        "Using the default event cache database for nvrid %s",
                        gParams.m_nvrid.c_str());
                }
                else
                {
                    // For N:2 failover, allow the following database names:
                    //   1. debug-primary.  Purpose: unit test
                    //   2. primary NVR's logical_id.
                    //      Client will specify an NVRID=NM<virtual-name> or NVRID=IP<virtual-ip>
                    //      Use the virtual-name or virtual-ip to looup the logical_id in the mediadb.

                    ADMEDIADB::Mediadatabase::Ptr mdb = ADMEDIADB::Mediadatabase::Ptr();

                    try
                    {
                        mdb = ADMEDIADB::Mediadatabase::CreatePooled("media", "itvtrack", "itvtrack");
                    }
                    catch (...)
                    {
                        syslog(
                            LOG_INFO,
                            "%s: %s: %d: Failed to create mediadatabase object",
                            __FILE_NAME__,
                            __FUNCTION__,
                            __LINE__);
                    }

                    std::string primaryNvrGlobalId;

                    if (mdb)
                    {
                        ADMEDIADB::FailoverNvr::Ptr priNvr
                            = ADMEDIADB::FailoverNvr::LocateByVirtualNameOrIpAddr(
                                mdb->GetRdbms(), nvridName, nvridAddress);

                        if (priNvr)
                        {
                            primaryNvrGlobalId = priNvr->GetGlobalId();
                        }
                    }

                    try
                    {
                        bool myActiveNvrid = false;

                        if (gParams.m_failoverState == FAILOVER_STATE_SECONDARY_ACTIVE)
                        {
                            videoedge::failover::CFailoverNvrUnitStatePtr failoverNvrUnitState
                                = videoedge::failover::CFailoverNvrUnitState::getFailoverNvrUnitState();
                            std::string failedNvrId;
                            if (failoverNvrUnitState->getFailoverSecondaryNvrStatePtr())
                            {
                                // Initialize sNvrID to the config.ini/logical_id of the failed primary NVR
                                failedNvrId
                                    = failoverNvrUnitState->getFailoverSecondaryNvrStatePtr()->getFailedNvrId();
                            }

                            myActiveNvrid
                                = (gParams.m_nvrid == "debug-primary" || (failedNvrId == primaryNvrGlobalId));
                        }
                        else
                        {
                            // Allow the site manager to recover primary events when the Secondary is in
                            // FAILOVER_STATE_SECONDARY_MONITORING state
                            myActiveNvrid = true;
                        }

                        // If a Primary event database is being used
                        if (myActiveNvrid)
                        {
                            std::string dbNameSuffix = gParams.m_nvrid;
                            if (gParams.m_nvrid != "debug-primary")
                            {
                                dbNameSuffix = primaryNvrGlobalId;
                                if (dbNameSuffix.empty())
                                {
                                    std::ostringstream oss;
                                    oss << "UnitNotificationRecoveryStream - Invalid nvrid parameter: "
                                        << gParams.m_nvrid << " No event database found for the nvrid.";

                                    syslog(LOG_INFO, "%s", oss.str().c_str());

                                    // Keep the web server happy on this failure. Just return END OF DATA
                                    std::cout << "Content-Type: text/xml\n\n";
                                    std::cout << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
                                    std::cout << "<!-- " << oss.str() << " -->" << std::endl;
                                    std::cout << "<!-- ENDDATA -->" << std::flush;
                                    return false;
                                }
                            }

                            // Override the default database name: Use the primary event cache database
                            dbPath = EventCacheDatabaseManager::GenerateFailoverDatabasePathFromNvrid(
                                dbNameSuffix);
                        }
                    }
                    catch (...)
                    {}
                }  // end else if not myStaticSecondaryIP
            }  // end if have an nvrid parameter
        }  // end if failover state is SECONDARY_ACTIVE or SECONDARY_MONITORING


        try
        {
            m_DB = new EventCacheDatabaseManager(
                gParams.m_fVerbose ? EventCacheDatabaseManager::diagnostics::enabled
                                   : EventCacheDatabaseManager::diagnostics::disabled,
                dbPath,
                SQLITE_OPEN_READONLY);
        }
        catch (const std::domain_error &ex)
        {
            syslog(LOG_ERR, "Failed to construct EventCacheDatabaseManager: %s", ex.what());
            return false;
        }
        catch (...)
        {
            syslog(LOG_ERR, "Unable to allocate EventCacheDatabaseManger. Out of memory.");
            return false;
        }
    }

    if (gParams.IsCGI() && gParams.m_fVerbose)
    {
        syslog(
            LOG_INFO,
            "ESS UnitNotificationRecoveryStream - Connected to ADDR:%s HOST:%s",
            gParams.m_REMOTE_ADDR.c_str(),
            gParams.m_REMOTE_HOST.c_str());
    }

    // Setup the initial conditions
    m_numRecords = gParams.m_nEnd - gParams.m_nStart + 1;
    m_iStart = gParams.m_nStart;
    m_iEnd = m_iStart + EcrecoveryHandler::m_BlockSize - 1;
    if (m_iEnd > gParams.m_nEnd)
    {
        m_iEnd = gParams.m_nEnd;
    }
    m_fQueryInProgress = false;

    // Flag the start time
    m_start_time = time(0);

    // Output the standard CGI startup content
    std::cout << "Content-Type: text/xml\n\n";
    std::cout << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";

    return true;
}

void EcrecoveryHandler::ProcessIdleTime()
{
    // If a new query needs to be started
    if (!m_fQueryInProgress)
    {
        // Start the query
        if (gParams.m_fVerbose)
        {
            std::ostringstream oss;
            oss << "UnitNotificationRecoveryStream - Start query from (" << m_iStart << " to " << m_iEnd
                << ") from range (" << gParams.m_nStart << " to " << gParams.m_nEnd << ")";
            syslog(LOG_INFO, "%s", oss.str().c_str());
        }

        // Grab the next block of records
        if (!m_DB->StartEventCacheQuery(m_iStart, m_iEnd))
        {
            std::ostringstream oss;
            oss << "UnitNotificationRecoveryStream - No data found in range ";
            oss << "(" << m_iStart << " to " << m_iEnd << ")";
            syslog(LOG_INFO, "%s", oss.str().c_str());

            TerminateTransmission();
        }
        else
        {
            m_fQueryInProgress = true;
        }
    }

    if (m_fQueryInProgress)
    {
        // Retrieve the next record.
        EventCache *pEventCache = m_DB->ReadNextEventCacheRecord();
        if (pEventCache)
        {
            std::string notify;
            if (pEventCache->FormatAsNotification(notify))
            {
                std::cout << notify;
                ++m_numRecordsSent;
            }
        }
        else
        {
            // End of block.
            m_fQueryInProgress = false;
            m_iStart = m_iEnd + 1;
            m_iEnd = m_iStart + EcrecoveryHandler::m_BlockSize - 1;
            if (m_iEnd > gParams.m_nEnd)
            {
                m_iEnd = gParams.m_nEnd;
            }
            if (m_iStart >= gParams.m_nEnd)
            {
                TerminateTransmission();
            }
        }
    }
}

void EcrecoveryHandler::TerminateTransmission()
{
    time_t end_time = time(0);
    time_t time_spent = end_time - m_start_time;

    // Only write to syslog if the operation took longer than a second
    // This is based on the current Victor site manager model of 10 record blocks
    // once per second.

    if (gParams.m_fVerbose || time_spent > 1)
    {
        // Format the final record count
        std::stringstream ss;
        ss << "Start: " << gParams.m_nStart << ", End: " << gParams.m_nEnd << ", Sent: " << m_numRecordsSent;

        ss << ", Time: " << time_spent << " Sec";

        // Terminate the dbus monitor loop
        syslog(
            LOG_INFO,
            "UnitNotificationRecoveryStream - End of data for ADDR:%s HOST:%s. %s",
            gParams.m_REMOTE_ADDR.c_str(),
            gParams.m_REMOTE_HOST.c_str(),
            ss.str().c_str());
    }

    // Terminate the data stream
    std::cout << "<!-- ENDDATA -->" << std::flush;

    gEventLoop.stop();
}
