// (c) 2010 Johnson Controls.  All Rights reserved.

#include "evbus_framework.h"
#include "evbus_fdmon.h"
#include "evbushandler.h"
#include "testectool.h"
#include "essmessage.h"
#include "libdz.h"

////////////////////////////////////////////////////////////////////////////
// Boost.Test boilerplate
// Load the boost test library as dynamic
// Create this test app's main()
//
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
///////////////////////////////////////////////////////////////////////////

/** Test modes */
enum TestStream
{
    Stream_UnitProperty = 0x01,
    Stream_UnitNotification = 0x02,
    Stream_UnitAlarm = 0x03
};

using namespace eventchannel;

/// The ownerid used for the tests
std::string strOwnerID("testectool");

CFileDescriptorEventLoop gEventLoop;

/// Ptr. to the run mode specific message handler.
EctoolMessageHandlerBase *gHandlerPtr = 0;

/// The total number of message groups to be sent.
int gnCount = 10;

/// The stream type to test.
unsigned int gnStreamType = Stream_UnitProperty;

/// The number of bytes to send as a payload with each message.
int gnBytes = 1024;

/// The listener loop timeout in seconds (DBUS Message Pump)
int gnTimeoutSecs = 1;

/// The listener timeout, micro seconds portion (DBUS Message Pump)
int gnTimeoutUSecs = 0;

/// The total number of test messages sent.
/// This is used by --runmode test --stream Xxxxxx mode.
int gnTotalMessageGroupsSent = 0;

void signalActionHandler(int signo)
{
    // Set a flag to terminate the DBUS event looop
    BOOST_TEST_MESSAGE("Terminating the DBUS event loop at your request.");
    gEventLoop.stop();
}

/// Fdmon timeout callback.
///
/// Called if the DBUS event loop sits idle for
/// more than the specified timout.
void handleFdmonIdle()
{
    std::string strAlarmName("AlarmName Field");
    std::string strEventName("EventName Field");
    std::string strValue("Value Field");
    std::string strInfo("Info Field");
    std::string strParamName("ParamName Field");
    std::string strParamValue("ParamValue Field");

    // TODO: If gnBytes > 0 then: strParamName="Bytes", strParamValue="encoded content"

    // Send one of each type of stream using the PublishESS.... functions.
    if (gnTotalMessageGroupsSent < gnCount)
    {
        PublishESSAlarm(
            strAlarmName.c_str(),
            strValue.c_str(),
            strInfo.c_str(),
            strParamName.c_str(),
            strParamValue.c_str(),
            0,
            NULL);
        PublishESSNotification(
            strAlarmName.c_str(),
            strValue.c_str(),
            strInfo.c_str(),
            strParamName.c_str(),
            strParamValue.c_str(),
            NULL);
        PublishESSChange(
            strAlarmName.c_str(),
            strValue.c_str(),
            strInfo.c_str(),
            strParamName.c_str(),
            strParamValue.c_str());
    }

    // Increment the counter
    ++gnTotalMessageGroupsSent;

    // Allow this to continue for nDelayCycles after all messages have been sent.
    int nDelayCycles = 5;

    showInt("FdmonIdle", gnTotalMessageGroupsSent);

    if (gnTotalMessageGroupsSent > (gnCount + nDelayCycles))
    {
        // Force the listener loop to exit
        gEventLoop.stop();
    }
}

void reportEvbusError(const char *msg)
{
    std::ostringstream os;

    if (msg)
        os << msg;
    else
        os << "Evbus error";
    os << " ";

    os << "errno = " << errno << ", ";

    const char *pszErr = EvbusStrerror(errno);
    if (pszErr)
    {
        os << "\"" << pszErr << "\"";
    }

    std::string buffer(os.str());
    BOOST_TEST_MESSAGE(buffer);
}


void setupAlarmWatch(const char *message)
{
    int nAlarmWatchRet = 0;

    nAlarmWatchRet = EvbusFrameworkWatchForAlarm(message);
    BOOST_REQUIRE(nAlarmWatchRet == 0);

    if (0 != nAlarmWatchRet)
    {
        reportEvbusError("EvbusFrameworkWatchForAlarm");
        EvbusTerminate();
        exit(2);
    }
}

void stopAlarmWatch(const char *message)
{
    int nAlarmWatchRet = 0;

    nAlarmWatchRet = EvbusFrameworkStopWatchingAlarm(message);
    BOOST_REQUIRE(nAlarmWatchRet == 0);

    if (0 != nAlarmWatchRet)
    {
        reportEvbusError("EvbusFrameworkStopWatchingAlarm");
        EvbusTerminate();
        exit(2);
    }
}

void setupEventWatch(const char *message)
{
    int nEventWatchRet = 0;

    nEventWatchRet = EvbusFrameworkWatchForEvent(message);
    BOOST_REQUIRE(nEventWatchRet == 0);

    if (0 != nEventWatchRet)
    {
        reportEvbusError("EvbusFrameworkWatchForEvent");
        EvbusTerminate();
        exit(2);
    }
}

void stopEventWatch(const char *message)
{
    int nEventWatchRet = 0;

    nEventWatchRet = EvbusFrameworkStopWatchingEvent(message);
    BOOST_REQUIRE(nEventWatchRet == 0);

    if (0 != nEventWatchRet)
    {
        reportEvbusError("EvbusFrameworkStopWatchingEvent");
        EvbusTerminate();
        exit(2);
    }
}

void setupRunModeFilters(unsigned int nStreamType)
{
    stopAlarmWatch(ESS_MSG_PROP_ALARM);
    stopEventWatch(ESS_MSG_PROP_NOTIFICATION);
    stopEventWatch(ESS_MSG_PROP_CHANGE);

    if (nStreamType == Stream_UnitProperty)
    {
        setupAlarmWatch(ESS_MSG_PROP_ALARM);
        setupEventWatch(ESS_MSG_PROP_NOTIFICATION);
        setupEventWatch(ESS_MSG_PROP_CHANGE);
    }
    else if (nStreamType == Stream_UnitNotification)
    {
        setupEventWatch(ESS_MSG_PROP_NOTIFICATION);
    }
    else if (nStreamType == Stream_UnitAlarm)
    {
        setupAlarmWatch(ESS_MSG_PROP_ALARM);
    }
}


/////////////////////////////////////////////////////////////////
void handleIncomingEvent(const char *source, EVBusEvent *event)
{
    if (gHandlerPtr)
    {
        gHandlerPtr->HandleEvbusEvent(source, event);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// #define DISPLAY_RAW_ALARM_DATA 1

void handleIncomingAlarm(const char *source, EvbusSeverity severity, EVBusEvent *event)
{
    if (gHandlerPtr)
    {
        gHandlerPtr->HandleEvbusAlarm(source, severity, event);
    }
}

bool createBinaryData(std::string &str, int nBytes)
{
    bool fOK = true;

    unsigned char *buf = 0;
    unsigned char *encodedBuf = 0;

    int nEncodedBytes = ((nBytes / BASE64_ENCODE_INPUT_LIMIT) + 1) * BASE64_ENCODE_OUTPUT_LIMIT;

    buf = (unsigned char *) calloc(nBytes, 1);
    if (buf)
    {
        encodedBuf = (unsigned char *) calloc(nEncodedBytes + 1, 1);
        if (encodedBuf)
        {
            int i = 0;
            unsigned char j = 0;
            // Pattern: [0][1][2]...[255][0][1][2]....
            while (i < nBytes)
            {
                for (j = 0; i < nBytes && j < 256; ++j)
                {
                    buf[i++] = j;
                }
            }
            base64Encode(nBytes, (char *) buf, (char *) encodedBuf);
        }
        else
        {
            fOK = false;
            std::cout << "Failed to allocate encodedBuf[" << nEncodedBytes << "]" << std::endl;
        }
    }
    else
    {
        fOK = false;
        std::cout << "Failed to allocate buf[" << nBytes << "]" << std::endl;
    }

    if (buf && encodedBuf)
    {
        str = (char *) encodedBuf;
    }
    else
    {
        str = "";
    }

    if (buf) free(buf);

    if (encodedBuf) free(encodedBuf);

    return fOK;
}


bool validateBinaryData(const std::string &str, int nBytesExpected)
{
    bool fOK = true;

    unsigned char *buf = (unsigned char *) calloc(nBytesExpected + 4, 1);
    buf[nBytesExpected + 0] = 3;
    buf[nBytesExpected + 1] = 2;
    buf[nBytesExpected + 2] = 1;
    buf[nBytesExpected + 3] = 0;

    int nBytes = base64Decode(const_cast<char *>(str.c_str()), (char *) buf, nBytesExpected);

    if (nBytes < nBytesExpected)
    {
        std::cout << "validateBinaryData: Expected " << nBytesExpected << ", base64Decode returned " << nBytes
                  << " - FAILED!" << std::endl;
    }

    // Check the guard bytes
    if (buf[nBytesExpected + 0] == 3 && buf[nBytesExpected + 1] == 2 && buf[nBytesExpected + 2] == 1
        && buf[nBytesExpected + 3] == 0)
    {
        fOK = false;
        std::cout << "validateBinaryData: buf guard bytes corrupted - FAILED!" << std::endl;
    }

    int i = 0;
    unsigned char j = 0;

    // Pattern: [0][1][2]...[255][0][1][2]....
    while (i < nBytesExpected && fOK)
    {
        for (j = 0; i < nBytesExpected && j < 256; ++j)
        {
            if (buf[i] != j)
            {
                fOK = false;
                std::cout << "validateBinaryData: buf[" << i << "] of " << nBytesExpected
                          << ": mismatch. - FAILED!" << std::endl;
                unsigned int c0 = buf[i];
                unsigned int c1 = buf[i];
                std::cout << "validateBinaryData: buf[" << i << "] == " << buf[i] << "(" << c0
                          << "), expected to find " << j << "(" << c1 << ")" << std::endl;
                break;
            }
            ++i;
        }
    }

    if (!fOK)
    {
        std::cout << "validateBinaryData: FAILED!" << std::endl;
    }

    if (buf) free(buf);

    return fOK;
}

// #define ENABLE_DIAGNOSTICS

void showInt(const char *pMsg, int value)
{
    std::ostringstream os;
    if (pMsg)
    {
        os << pMsg << " = " << value;
    }
    else
    {
        os << "Value = " << value;
    }
    std::string buffer(os.str());

#ifdef ENABLE_DIAGNOSTICS
    BOOST_TEST_MESSAGE(buffer);
#endif
}

void showBool(const char *pMsg, bool value)
{
    showInt(pMsg, value ? 1 : 0);
}

void showString(const char *pMsg, const char *value)
{
    std::ostringstream os;
    if (pMsg)
    {
        os << pMsg << " = " << value;
    }
    else
    {
        os << value;
    }
    std::string buffer(os.str());

#ifdef ENABLE_DIAGNOSTICS
    BOOST_TEST_MESSAGE(buffer);
#endif
}

///////////////////////////////////////////////////////////////////////////
bool EctoolTestHandler::ProcessMessage(EventChannelMessage *pMsg)
{
    bool fOK = true;

    if (!pMsg)
    {
        fOK = false;
    }
    else
    {
        std::ostringstream os;

        showString("EctoolTestHandler::ProcessMessage", pMsg->Name().c_str());

        // Check for message validity
        bool fAlarm = pMsg->Type() != MT_Event;
        bool fESS_PropAlarm = (pMsg->Name() == ESS_MSG_PROP_ALARM);
        bool fESS_PropChange = (pMsg->Name() == ESS_MSG_PROP_CHANGE);
        bool fESS_PropNotification = (pMsg->Name() == ESS_MSG_PROP_NOTIFICATION);
        bool fNonESS = (!fESS_PropAlarm && !fESS_PropChange && !fESS_PropNotification);


        // showBool( "fAlarm", fAlarm );
        // showBool( "fESS_PropAlarm", fESS_PropAlarm );
        // showBool( "fESS_PropChange", fESS_PropChange );
        // showBool( "fESS_PropNotification", fESS_PropNotification );
        // showBool( "fNonESS", fNonESS );

        if (fNonESS)
        {
            ++m_nInvalidMessageCount;
            return false;
        }


        if (m_nStreamType == Stream_UnitNotification && fAlarm)
        {
            ++m_nInvalidInterfaceCount;
            return false;
        }

        if (m_nStreamType == Stream_UnitAlarm && !fAlarm)
        {
            ++m_nInvalidInterfaceCount;
            return false;
        }

        if (m_nStreamType == Stream_UnitProperty)
        {
            ++m_nValidMessageCount;
            return true;
        }

        if (m_nStreamType == Stream_UnitAlarm)
        {
            if (fESS_PropChange || fESS_PropNotification)
            {
                ++m_nInvalidESSMessageCount;
                return false;
            }
            else
            {
                ++m_nValidMessageCount;
                return true;
            }
        }

        if (m_nStreamType == Stream_UnitNotification)
        {
            if (fESS_PropChange || fESS_PropAlarm)
            {
                ++m_nInvalidESSMessageCount;
                return false;
            }
            else
            {
                ++m_nValidMessageCount;
                return true;
            }
        }
    }
    return fOK;
}

bool RunCurrentTest()
{
    bool fConnected = true;

    // Allocate a new test handler
    EctoolTestHandler *pTestHandler = 0;
    if (gHandlerPtr)
    {
        delete gHandlerPtr;
        gHandlerPtr = 0;
    }

    gHandlerPtr = pTestHandler = new EctoolTestHandler;

    // Setup the stream type and expected message count
    int nExpectedCount = gnCount;
    if (gnStreamType == Stream_UnitProperty)
    {
        nExpectedCount = gnCount * 3;
    }
    pTestHandler->setExpectedMessageCount(nExpectedCount);
    pTestHandler->setStreamType(gnStreamType);

    EvbusSetVerbose(0);

    // Connect to the bus if required
    if (!EvbusHasConnection())
    {
        int nConnectRet = EvbusInitialize(0, strOwnerID.c_str());
        BOOST_REQUIRE(nConnectRet == 0);
        if (0 != nConnectRet)
        {
            reportEvbusError("EvbusInitialize");
            fConnected = false;
            return fConnected;
        }


        // Setup subscriber information
        int nFrameworkRet = EvbusFrameworkInitialize(handleIncomingEvent, handleIncomingAlarm);
        BOOST_REQUIRE(nFrameworkRet == 0);
        if (0 != nFrameworkRet)
        {
            reportEvbusError("EvbusFrameworkInitialize");
            EvbusTerminate();
            fConnected = false;
            return fConnected;
        }
    }

    setupRunModeFilters(gnStreamType);

    if (gHandlerPtr)
    {
        if (gHandlerPtr->DoPreprocess())
        {
            showString(0, "Starting the DBUS listener event loop");

            const auto timeout = std::chrono::milliseconds(gnTimeoutSecs * 1000 + gnTimeoutUSecs / 1000);
            gEventLoop.run(EvbusFrameworkPublishFds, EvbusFrameworkNotify, handleFdmonIdle, timeout);

            // We get here when the listener loop termiantes.
            // Time to check the results
            EctoolTestHandler *pH = (EctoolTestHandler *) gHandlerPtr;

            showInt("Expected message count", pH->m_nExpectedMessageCount);
            showInt("Invalid interface count", pH->m_nInvalidInterfaceCount);
            showInt("Invalid message count", pH->m_nInvalidMessageCount);
            showInt("Invalid ESS message count", pH->m_nInvalidESSMessageCount);
            showInt("Valid message count", pH->m_nValidMessageCount);
            showInt("Stream type", pH->m_nStreamType);

            BOOST_CHECK_MESSAGE(pH->m_nInvalidInterfaceCount == 0, "Messages received on the wrong interface");
            BOOST_CHECK_MESSAGE(
                pH->m_nInvalidMessageCount == 0, "Non ESS Messages received. Message filter failure.");
            BOOST_CHECK_MESSAGE(
                pH->m_nInvalidESSMessageCount == 0, "Incorrect ESS messages received. Invalid filter setup.");
            BOOST_CHECK_MESSAGE(
                pH->m_nValidMessageCount > 0, "No valid messages recevied. Event Manager may not be running.");
            BOOST_CHECK_MESSAGE(
                pH->m_nValidMessageCount >= pH->m_nExpectedMessageCount,
                "Messages dropped. Possible system overload.");
        }
    }
    return fConnected;
}

BOOST_AUTO_TEST_CASE(Startup)
{
    boost::unit_test::unit_test_log.set_stream(std::cout);

    showString(0, "Startup - connecting signal handlers.");

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
}

BOOST_AUTO_TEST_CASE(Basic_UnitPropertyStream_Test)
{
    showString(0, "Basic_UnitPropertyStream_Test");

    gnCount = 100;
    gnStreamType = Stream_UnitProperty;
    gnBytes = 0;
    gnTimeoutSecs = 0;
    gnTimeoutUSecs = 125000;
    gnTotalMessageGroupsSent = 0;

    RunCurrentTest();
}

BOOST_AUTO_TEST_CASE(Basic_UnitAlarmStream_Test)
{
    showString(0, "Basic_UnitAlarmStream_Test");

    gnCount = 100;
    gnStreamType = Stream_UnitAlarm;
    gnBytes = 0;
    gnTimeoutSecs = 0;
    gnTimeoutUSecs = 125000;
    gnTotalMessageGroupsSent = 0;

    RunCurrentTest();
}

BOOST_AUTO_TEST_CASE(Basic_UnitNotificationStream_Test)
{
    showString(0, "Basic_UnitNotificationStream_Test");

    gnCount = 100;
    gnStreamType = Stream_UnitNotification;
    gnBytes = 0;
    gnTimeoutSecs = 0;
    gnTimeoutUSecs = 125000;
    gnTotalMessageGroupsSent = 0;

    RunCurrentTest();
}

BOOST_AUTO_TEST_CASE(UnitPropertyStream_Test_10K)
{
    showString(0, "Basic_UnitPropertyStream_Test");

    gnCount = 10000;
    gnStreamType = Stream_UnitProperty;
    gnBytes = 0;
    gnTimeoutSecs = 0;
    gnTimeoutUSecs = 100;
    gnTotalMessageGroupsSent = 0;

    RunCurrentTest();
}
