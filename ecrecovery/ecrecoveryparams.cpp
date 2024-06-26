///////////////////////////////////////////////////////////////////////////////////////
///
/// (c) 2010 Tyco International Ltd. and its respective companies. All rights reserved.
///
///////////////////////////////////////////////////////////////////////////////////////

/******************************************************************/
// ectool - Parameters
//
/*******************************************************************/


#include <cstdio>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <getopt.h>
#include <limits.h>
#include "ecrecoveryparams.h"
#include "essmessage.h"
#include "utilTimestamp.h"
#include "evbus.h"
#include "failover.h"
#include "failoverNvrState.h"
#include "syslog.h"

#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>

const char *Cmd_id = "id";
const char Cmd_id_short = 'i';
const char *Cmd_timeout = "timeout";
const char Cmd_timeout_short = 't';
const char *Cmd_help = "help";
const char Cmd_help_short = 'h';
const char *Cmd_extralogging = "logging";
const char Cmd_extralogging_short = 'L';
const char *Cmd_verbose = "verbose";
const char Cmd_verbose_short = 'v';
const char *Cmd_start = "start";
const char Cmd_start_short = 's';
const char *Cmd_end = "end";
const char Cmd_end_short = 'e';
const char *Cmd_database = "database";
const char Cmd_database_short = 'd';
const char *Cmd_nvrid = "nvrid";
const char Cmd_nvrid_short = 'n';
const char *Cmd_failoverstate = "failoverstate";
const char Cmd_failoverstate_short = 'F';

using namespace videoedge::util;
using namespace videoedge::failover;

struct option EcrecoveryParameters::m_longopts[]
    = {{Cmd_id, required_argument, 0, Cmd_id_short},
       {Cmd_timeout, required_argument, 0, Cmd_timeout_short},
       {Cmd_help, no_argument, 0, Cmd_help_short},
       {Cmd_verbose, no_argument, 0, Cmd_verbose_short},
       {Cmd_extralogging, no_argument, 0, Cmd_extralogging_short},
       {Cmd_start, required_argument, 0, Cmd_start_short},
       {Cmd_end, required_argument, 0, Cmd_end_short},
       {Cmd_database, required_argument, 0, Cmd_database_short},
       {Cmd_nvrid, required_argument, 0, Cmd_nvrid_short},
       {Cmd_failoverstate, required_argument, 0, Cmd_failoverstate_short},

       // The following marker entry is required and must be last.
       {0, 0, 0, 0}};

void EcrecoveryParameters::ShowParameters()
{
    ShowParameter("Owner ID", m_fOwnerID, m_strOwnerID);
    ShowParameter("Idle Timeout (secs)", m_fTimeout, &m_timeout);
    ShowParameter("Verbose", m_fVerbose);
    ShowParameter("Extra logging", m_fExtraLogging);
    ShowParameter("Start seq", m_fStart, m_nStart);
    ShowParameter("End seq", m_fEnd, m_nEnd);
    ShowParameter("Database", m_fDatabase, m_strDatabase);
    ShowParameter("nvrid", m_fNvrid, m_nvrid);
    ShowParameter("Failover state", m_fFailoverState, m_failoverState);
    ShowEOL();
}

void EcrecoveryParameters::ShowParameter(const char *name, bool fUserSet, const std::string &str)
{
    std::cout << name << " ";
    if (fUserSet)
    {
        std::cout << "User value: ";
    }
    else
    {
        std::cout << "Default value: ";
    }
    std::cout << "\"" << str << "\"";
    ShowEOL();
}

void EcrecoveryParameters::ShowParameter(const char *name, bool fUserSet, int n)
{
    std::cout << name << " ";
    if (fUserSet)
    {
        std::cout << "User value: ";
    }
    else
    {
        std::cout << "Default value: ";
    }
    std::cout << n;

    ShowEOL();
}

void EcrecoveryParameters::ShowParameter(const char *name, bool fUserSet)
{
    std::cout << name << " ";
    if (fUserSet)
    {
        std::cout << "Yes";
    }
    else
    {
        std::cout << "No";
    }
    ShowEOL();
}

void EcrecoveryParameters::ShowParameter(const char *name, bool fUserSet, struct timeval *tv)
{
    std::cout << name << " ";
    if (fUserSet)
    {
        std::cout << "User value: ";
    }
    else
    {
        std::cout << "Default value: ";
    }
    std::cout << tv->tv_sec << " Sec, " << tv->tv_usec << " uSec";
    ShowEOL();
}

bool EcrecoveryParameters::ParseArgs(int argc, char *argv[])
{
    bool fOK = false;

    // Command line parameters override CGI (QUERY_STRING)
    if (argc <= 1)
    {
        fOK = ParseCGI();
    }
    else
    {
        fOK = ParseCommandLine(argc, argv);
    }

    if (fOK) fOK = ValidateParams();

    return fOK;
}

bool EcrecoveryParameters::ParseCommandLine(int argc, char *argv[])
{
    char shortopts_buf[256];

    const char *shortopts = shortopts_buf;

    bool fOK = true;

    int idx = -1;

    m_strLastError = "";

    // Construct the shortopts buffer
    unsigned int j = 0;
    for (unsigned int i = 0; EcrecoveryParameters::m_longopts[i].name != 0; ++i)
    {
        shortopts_buf[j++] = (char) EcrecoveryParameters::m_longopts[i].val;
        if (EcrecoveryParameters::m_longopts[i].has_arg == required_argument)
        {
            shortopts_buf[j++] = ':';
        }
    }
    shortopts_buf[j++] = '\0';

    while (1)
    {
        int opt = getopt_long(argc, argv, shortopts, m_longopts, &idx);
        if (opt == -1) break;  // End of options

            // optarg is a Ptr. to a string containing the argument parameter.
#if 0
    // Diagnostics code:
    char szCmd[2];
    szCmd[0] = opt;
    szCmd[1] = 0;
    if( optarg )
    {
      std::cout << "option (" << szCmd << ") " << optarg << std::endl;
    }else
    {
      std::cout << "option (" << szCmd << ") no parameter specified." << std::endl;
    }
#endif

        switch (opt)
        {
        case Cmd_id_short:  // ownder id
            m_fOwnerID = true;
            m_strOwnerID = optarg;
            break;
        case Cmd_timeout_short:  // timeout
        {
            // Convoluted conversion to eliminate roundoff errors
            double dSecs = atof(optarg);
            int wholeSecs = (int) dSecs;
            double fracSecs = dSecs - (float) wholeSecs;
            double uSecs = fracSecs * 1.0E6 + 0.0000005;
            m_timeout.tv_sec = (int) dSecs;
            m_timeout.tv_usec = (int) uSecs;
            m_fTimeout = true;
        }
        break;
        case Cmd_help_short:  // help
            m_fHelp = true;
            break;
        case Cmd_verbose_short:  // verbose
            m_fVerbose = true;
            break;
        case Cmd_extralogging_short:  // Extra logging
            m_fExtraLogging = true;
            break;
        case Cmd_start_short:  // Start seq
        {
            m_fStart = true;
            std::stringstream ss(optarg);
            ss >> m_nStart;
        }
        break;
        case Cmd_end_short:  // end seq
        {
            m_fEnd = true;
            std::stringstream ss(optarg);
            ss >> m_nEnd;
        }
        break;

        case Cmd_database_short:  // datbase
            m_fDatabase = true;
            m_strDatabase = optarg;
            break;

        case Cmd_nvrid_short:
            m_nvrid = optarg;
            m_fNvrid = true;
            break;

        case Cmd_failoverstate_short:
            m_failoverState = atoi(optarg);
            m_fFailoverState = true;
            break;

        default:
            break;
        };  // end switch opt
    };  // end while


    return fOK;
}

bool EcrecoveryParameters::ParseCGI()
{
    bool fOK = true;
    cgicc::form_iterator it;
    std::string strName;
    std::string strValue;
    std::string strLastParamName;
    cgicc::Cgicc cgi;

    // Attempt to gather some information about the client connecting to this CGI
    if (getenv("REMOTE_ADDR"))
        m_REMOTE_ADDR = getenv("REMOTE_ADDR");
    else
        m_REMOTE_ADDR = "";

    if (getenv("REMOTE_HOST"))
        m_REMOTE_HOST = getenv("REMOTE_HOST");
    else
        m_REMOTE_HOST = "";

    m_fCGIMode = true;
    strLastParamName = "";

    it = cgi.getElement(Cmd_start);
    if (it != cgi.getElements().end() && !it->isEmpty())
    {
        m_fStart = true;
        std::stringstream ss((*it).getValue());
        ss >> m_nStart;
    }

    it = cgi.getElement(Cmd_end);
    if (it != cgi.getElements().end() && !it->isEmpty())
    {
        m_fEnd = true;
        std::stringstream ss((*it).getValue());
        ss >> m_nEnd;
    }

    it = cgi.getElement(Cmd_timeout);
    if (it != cgi.getElements().end() && !it->isEmpty())
    {
        strValue = (*it).getValue();

        // Convoluted conversion to eliminate roundoff errors
        double dSecs = atof(strValue.c_str());
        int wholeSecs = (int) dSecs;
        double fracSecs = dSecs - (float) wholeSecs;
        double uSecs = fracSecs * 1.0E6 + 0.0000005;
        m_timeout.tv_sec = (int) dSecs;
        m_timeout.tv_usec = (int) uSecs;
        m_fTimeout = true;
    }

    it = cgi.getElement(Cmd_nvrid);
    if (it != cgi.getElements().end() && !it->isEmpty())
    {
        m_nvrid = (*it).getValue();
        m_fNvrid = true;
    }

    it = cgi.getElement(Cmd_failoverstate);
    if (it != cgi.getElements().end() && !it->isEmpty())
    {
        m_failoverState = atoi((*it).getValue().c_str());
        m_fFailoverState = true;
    }

    return fOK;
}

bool EcrecoveryParameters::ValidateParams()
{
    bool fOK = true;

    m_strLastError = "";

    // If no Owner ID is specified, then create one
    if (!m_fOwnerID)
    {
        char tmpID[50];
        m_fOwnerID = true;
        m_strOwnerID = EvbusCreateOwnerid("ECR", tmpID, sizeof(tmpID));
    }

    if (!m_fTimeout)
    {
        // Keep the default idle timeout short to reduce DB lock time
        m_timeout.tv_sec = 0;
        m_timeout.tv_usec = 10;  // 100 uS == .01 ms
    }

    if (!m_fStart || !m_fEnd)
    {
        std::ostringstream oss;

        if (!m_fStart && !m_fEnd)
        {
            oss << "Missing start and end parameters.";
        }
        else if (!m_fStart)
        {
            oss << "Missing start parameter.";
        }
        else if (!m_fEnd)
        {
            oss << "Missing end parameter.";
        }
        m_strLastError = oss.str();
        fOK = false;
    }

    if (fOK)
    {
        std::ostringstream oss;

        if (m_nStart > m_nEnd)
        {
            oss << "The start parameter must be less than the end parameter.";
            fOK = false;
        }
        else if (m_nStart < 0 || m_nEnd < 0)
        {
            oss << "The start and end parameters must be in the range 1..N";
            fOK = false;
        }
        if (!fOK)
        {
            m_strLastError = oss.str();
        }
    }

    // Gather the appropriate failover state info
    // If no failover state was sepcified via the command line
    if (!m_fFailoverState)
    {
        // Get the actual failover state
        m_failoverState = getFailoverState();
    }
    else
    {
        syslog(LOG_INFO, "NOTICE: Emulating failover state %d", m_failoverState);
    }

    return fOK;
}

void EcrecoveryParameters::ShowUsage()
{
    std::cout << "Usage:" << std::endl << std::endl;

    std::cout << "ecrecovery [<parameters>] [<name-value>...]" << std::endl << std::endl;


    std::cout << "Where:" << std::endl;
    std::cout << "<parameters> are:" << std::endl;
    std::cout << "   Option            Parameter   Description" << std::endl;

    std::cout << "   --id|-i           ID          The owner ID for the DBUS connection" << std::endl;
    std::cout << "   --timeout|-t      seconds     The idle timeout for subscribers (Example: 1.55)."
              << std::endl;
    std::cout << "   --verbose|-v                  Generate maximum diagnotics trace information." << std::endl;
    std::cout << "   --help|-h                     Show this help text." << std::endl;
    std::cout << std::endl;
    std::cout << "   --start          seqNo       The starting sequence number." << std::endl;
    std::cout << "   --end            seqNo       The ending sequence number." << std::endl;
    std::cout << "   --database       dbname      Specify a non-standard database. Used for testing."
              << std::endl;
    std::cout << std::endl;
}

void EcrecoveryParameters::ShowLastError()
{
    std::cerr << m_strLastError;
}

void EcrecoveryParameters::ShowEOL()
{
    if (IsCGI())
    {
        std::cout << "<br/>";
    }
    else
    {
        std::cout << std::endl;
    }
}
