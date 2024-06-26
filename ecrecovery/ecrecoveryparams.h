// (c) 2010 Johnson Controls.  All Rights reserved.

#ifndef _ECRECOVERYPARAMETERS_H_
#define _ECRECOVERYPARAMETERS_H_

#include "sqlite3.h"

/**
 * Encapsulates the ectool parameters.<br>
 * Parameters populated via argc,argv or via the CGI QUERY_STRING environment variable
 */
class EcrecoveryParameters
{
public:
    EcrecoveryParameters()
        : m_fOwnerID(false),
          m_fTimeout(false),
          m_fHelp(false),
          m_fVerbose(false),
          m_fCGIMode(false),
          m_fExtraLogging(false),
          m_fStart(false),
          m_nStart(0),
          m_fEnd(false),
          m_nEnd(0),
          m_fNvrid(false),
          m_fFailoverState(false),
          m_failoverState(0),
          m_fDatabase(false)
    {
        // Default timeout == 1000uS == 1ms
        m_timeout.tv_sec = 0;
        m_timeout.tv_usec = 1000;
    }

    /**
     * Parse the input parameters. The parameters will be read from the command line arguments.
     * @param argc The command line parameter count.
     * @param argv Ptr. to the array of command line parameter strings.
     * @returns bool Returns true if successful.
     *
     */
    bool ParseArgs(int argc, char *argv[]);

private:
    /**
     * Parse CGI parameters
     * @returns bool Returns true if successful.
     */
    bool ParseCGI();

    /**
     * Parse command line arguments.
     * @param argc The command line parameter count.
     * @param argv Ptr. to the array of command line parameter strings.
     * @returns bool Returns true if successful.
     */
    bool ParseCommandLine(int argc, char *argv[]);

    /**
     * Validate the parameters.<br>
     * Sets m_strLastError on failure.
     * @return bool Returns true if all is well. Returns false on failure.
     */
    bool ValidateParams();

public:
    bool IsCGI() const { return m_fCGIMode; }
    bool ExtraLogging() const { return m_fExtraLogging; }
    void ShowUsage();
    const std::string &GetLastError() const { return m_strLastError; }
    void ShowLastError();

    /**
     * Show the current parameter set in human friendly format.
     */
    void ShowParameters();

private:
    /** Show a single parameter */
    void ShowParameter(const char *name, bool fUserSet, const std::string &str);
    void ShowParameter(const char *name, bool fUserSet, int n);
    void ShowParameter(const char *name, bool fUserSet);
    void ShowParameter(const char *name, bool fUserSet, struct timeval *tv);
    void ShowEOL();

public:
    bool m_fOwnerID; /**< Flag: OwnerID specified */
    std::string m_strOwnerID; /**< The owner ID */

    bool m_fCamIndex = false; /**< Flag: Camera index specified */
    unsigned int m_nCamIndex = 0; /**< The camera index */

    bool m_fRunMode = false; /**< Flag: RunMode specified */
    unsigned int m_nRunMode = 0; /**< The run mode */

    bool m_fPublish = false; /**< Flag: Publish messages */
    unsigned int m_nPublishToInterface = 0; /**< The outgoing interface */

    bool m_fSubscribe = false; /**< Flag: Subscribe to messages */
    unsigned int m_nSubscribeToInterface = 0; /**< The inbound interfaces */

    bool m_fTimeout; /**< Flag: Timeout specified */
    timeval m_timeout; /**< The FDMon timeout time */

    bool m_fCGI = false; /**< CGI generation specified */

    bool m_fHelp; /**< Flag: Help requested */

    bool m_fVerbose; /**< Flag: Verbose diagnostics output */

private:
    /** Flag: The commands passed into the app were parsed from the CGI QUERY_STRING */
    bool m_fCGIMode;

    /** Flag: Generate extra syslog information */
    bool m_fExtraLogging;

public:
    /** Starting sequence number */
    bool m_fStart;
    sqlite3_int64 m_nStart;

    /** endinf sequence number */
    bool m_fEnd;
    sqlite3_int64 m_nEnd;

    bool m_fNvrid; /**< Flag: nvrid specified */
    std::string m_nvrid; /**< The target nvrid (Used by runmode ess and runmode evtmgr testing) */

    bool m_fFailoverState; /**< Flag: Failover state override. Debug use only */
    int m_failoverState; /**< The failover state. This overrides the actual failover state of the NVR */

    /** event cache database */
    bool m_fDatabase;
    std::string m_strDatabase;

public:
    std::string m_strLastError; /**< The last known error */

public:
    std::string m_REMOTE_ADDR; /**< The CGI remote addr */
    std::string m_REMOTE_HOST; /**< The CGI remote host */

public:
    /** The option list */
    static struct option m_longopts[];
};

#endif
