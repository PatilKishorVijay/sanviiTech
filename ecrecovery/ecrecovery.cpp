// (c) 2010 Johnson Controls.  All Rights reserved.

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include "evbus.h"
#include "evbus_framework.h"
#include "evbus_fdmon.h"
#include "ecrecovery.h"
#include "ecrecoveryparams.h"
#include "evbushandler.h"
#include "utilTimestamp.h"
#include "ecrecoveryhandler.h"

using namespace eventchannel;
using namespace videoedge::util;

/** The execution parameters:
 *  This object handles parsing the input parameters for extool for both command line
 *  and CGI use cases.<br>
 *  The final set of parameters are stored here as well.
 */
EcrecoveryParameters gParams;

CFileDescriptorEventLoop gEventLoop;

/**
 * The CGI "keep alive" hearbeat period.<br>
 * This is used in handleFdmonIdle() to output
 * XHTML comments to keep Apache from dropping the
 * connection.
 */
double gCGIHeartbeatSecs = 5.0;

/// Ptr. to the run mode specific message handler.
EctoolMessageHandlerBase *gHandlerPtr = 0;

void signalActionHandler(int signo)
{
    // Set a flag to terminate the DBUS event looop
    if (!gParams.m_fVerbose)
    {
        std::cout << "Terminating the DBUS event loop at your request." << std::endl;
    }
    gEventLoop.stop();
}

/**
 * Fdmon timeout callback.<br>
 * This function is called if the DBUS event loop sits idle for
 * more than the specified timout. If ectool is running in CGI mode,
 * then this function outputs an HTML comment to keep the apache server
 * from closing the connection.
 */
void handleFdmonIdle()
{
    static time_t lastHeartbeatSent = 0;

    // If there's a handler, then give it a time slice
    if (gHandlerPtr)
    {
        gHandlerPtr->ProcessIdleTime();
    }

    // Determine if it's time to issue an "I'm still alive message"
    time_t timeNow = time(0);
    double deltaT = difftime(timeNow, lastHeartbeatSent);

    // std::cout << "deltaT " << deltaT << std::endl;

    if (deltaT > gCGIHeartbeatSecs)
    {
        lastHeartbeatSent = timeNow;
        if (gParams.IsCGI())
        {
            std::cout << "<!-- NVR Event Channel Active -->" << std::flush;
        }
        else
        {
            if (!gParams.m_fVerbose)
            {
                std::cout << "Waiting for events..." << std::endl;
            }
        }
    }
}

bool sendEvbusMessage(EVBusEvent *message, bool fKeepGoing)
{
    bool fOK = true;

    int nRet = EvbusSendEvent(message);

    if (0 != nRet)
    {
        reportEvbusError("EvbusSendEvent");

        EvbusEventFree(message);

        fOK = fKeepGoing ? true : false;
    }
    return fOK;
}

void reportEvbusError(const char *msg)
{
    // Grab errno before any output
    int errCode = errno;

    // extern char  g_lastEvbusLog[];

    if (gParams.m_fVerbose)
    {
        std::cerr << "<!-- ";

        if (msg)
            std::cerr << msg;
        else
            std::cerr << "Evbus error";
        std::cerr << " ";

        std::cerr << "errno = " << errCode << ", ";

        const char *pszErr = EvbusStrerror(errCode);
        if (pszErr)
        {
            std::cerr << "\"" << pszErr << "\"";
        }
        std::cerr << " -->" << std::endl;
    }

    {
        const char *pszErr = EvbusStrerror(errCode);
        if (pszErr)
        {
            syslog(LOG_INFO, "ectool Evbus error %d. (%s)", errCode, pszErr);
        }
    }
}

void setupAlarmWatch(const char *message)
{
    int nRet = 0;

    nRet = EvbusFrameworkWatchForAlarm(message);
    if (0 != nRet)
    {
        reportEvbusError("EvbusFrameworkWatchForAlarm");
        EvbusTerminate();
        exit(2);
    }
}

void setupEventWatch(const char *message)
{
    int nRet = 0;

    nRet = EvbusFrameworkWatchForEvent(message);
    if (0 != nRet)
    {
        reportEvbusError("EvbusFrameworkWatchForEvent");
        EvbusTerminate();
        exit(2);
    }
}

void setupRunModeFilters()
{
    // TODO: pfossey - setupAlarmWatch/setupEventWatch to filter out noise
}

int main(int argc, char *argv[])
{
    bool fArgsOK = gParams.ParseArgs(argc, argv);

    if (gParams.m_fHelp)
    {
        gParams.ShowUsage();
    }
    else if (!fArgsOK)
    {
        if (gParams.m_fVerbose)
        {
            std::cout << "ecrecovery - Error: Invalid or missing parameters." << std::endl;
            gParams.ShowLastError();

            gParams.ShowUsage();
        }
        syslog(LOG_INFO, "ecrecovery error %s", gParams.GetLastError().c_str());
        exit(1);
    }

    sigset_t masked;
    sigfillset(&masked);
    sigdelset(&masked, SIGHUP);
    sigdelset(&masked, SIGTERM);
    sigdelset(&masked, SIGINT);
    sigprocmask(SIG_BLOCK, &masked, NULL);

    struct sigaction act;
    act.sa_handler = signalActionHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    if (gParams.m_fVerbose)
    {
        gParams.ShowParameters();
    }

    // Create a new EcrecoveryHandler here
    gHandlerPtr = new EcrecoveryHandler;


    EvbusSetVerbose(gParams.m_fVerbose ? 1 : 0);

    // Connect to the bus
    int nRet = EvbusInitialize(0, gParams.m_strOwnerID.c_str());
    if (0 != nRet)
    {
        reportEvbusError("EvbusInitialize");
        exit(2);
    }

    nRet = EvbusFrameworkInitialize(handleIncomingEvent, handleIncomingAlarm);
    if (0 != nRet)
    {
        reportEvbusError("EvbusFrameworkInitialize");
        EvbusTerminate();
        exit(2);
    }

    // Define the watch filters based on the current run mode
    setupRunModeFilters();

    // Handle any preprocessing required by the current handler
    if (gHandlerPtr)
    {
        if (gHandlerPtr->DoPreprocess())
        {
            if (gParams.IsCGI() && gParams.m_fVerbose)
            {
                // Issue a syslog to note the fact that an ESS connection is going down.
                syslog(
                    LOG_INFO,
                    "UnitNotificationRecoveryStream - Connected to ADDR:%s HOST:%s",
                    gParams.m_REMOTE_ADDR.c_str(),
                    gParams.m_REMOTE_HOST.c_str());
            }

            const timeval &t = gParams.m_timeout;
            const auto timeout = std::chrono::milliseconds(t.tv_sec * 1000 + t.tv_usec / 1000);
            gEventLoop.run(EvbusFrameworkPublishFds, EvbusFrameworkNotify, handleFdmonIdle, timeout);
        }
    }

    if (gParams.IsCGI() && gParams.m_fVerbose)
    {
        // Issue a syslog to note the fact that an ESS connection is going down.
        syslog(
            LOG_INFO,
            "UnitNotificationRecoveryStream - Disconnected from ADDR:%s HOST:%s",
            gParams.m_REMOTE_ADDR.c_str(),
            gParams.m_REMOTE_HOST.c_str());
    }

    // Disconnect from the bus
    EvbusTerminate();

    return 0;
}

void handleIncomingEvent(const char *source, EVBusEvent *event)
{
    if (gHandlerPtr)
    {
        if (gParams.m_fVerbose)
        {
            std::cout << "handleIncomingEvent from " << source << std::endl;
        }
        gHandlerPtr->HandleEvbusEvent(source, event);
    }
}

void handleIncomingAlarm(const char *source, EvbusSeverity severity, EVBusEvent *event)
{
    if (gHandlerPtr)
    {
        if (gParams.m_fVerbose)
        {
            std::cout << "handleIncomingAlarm from " << source << std::endl;
        }
        gHandlerPtr->HandleEvbusAlarm(source, severity, event);
    }
}
