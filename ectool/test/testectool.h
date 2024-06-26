///////////////////////////////////////////////////////////////////////////////////////
///
/// (c) 2010 Tyco International Ltd. and its respective companies. All rights reserved.
///
///////////////////////////////////////////////////////////////////////////////////////

/** Establish a connection to the DBUS and run the test */
bool RunCurrentTest();

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
void stopAlarmWatch(const char *message);

/** Set a watch on an event<br>
 * If this call fails, then the error is reported and ectool exits after closing the DBUS connection.
 * @param message Ptr. to nul terminated message name.
 * @returns none
 */
void setupEventWatch(const char *message);
void stopEventWatch(const char *message);


/** Setup RunMode specific filters */
void setupRunModeFilters(unsigned int nStreamType);

/** Create a string containing encoded binary data */
bool createBinaryData(std::string &str, int nBytes);

/** Validate binary data */
bool validateBinaryData(const std::string &str, int nBytesExpeted);

/** Output "Message: <string> via BOOST_TEST_MESSAGE */
void showString(const char *pMsg, const char *value);

/** Output "Message: <int>" via BOOST_TEST_MESSAGE */
void showInt(const char *pMsg, int value);

/** Output "Message: <bool> vis BOOST_TEST_MESSAGE */
void showBool(const char *pMsg, bool value);

/////////////////////////////////////////////////////////////////////
using namespace eventchannel;
/**
 * The message handler for ectool testing.
 */
class EctoolTestHandler : public EctoolMessageHandlerBase
{
public:
    /** ctor */
    EctoolTestHandler()
        : EctoolMessageHandlerBase(),
          m_nExpectedMessageCount(0),
          m_nInvalidInterfaceCount(0),
          m_nInvalidMessageCount(0),
          m_nInvalidESSMessageCount(0),
          m_nValidMessageCount(0),
          m_nStreamType(0)
    {}

    /** Set the expected valid mesage count */
    void setExpectedMessageCount(int n) { m_nExpectedMessageCount = n; }

    /** Set the subscribed stream type */
    void setStreamType(int n) { m_nStreamType = n; }

    /** Message processor - Overloaded from base class */
    virtual bool ProcessMessage(EventChannelMessage *pMsg);

    /** The number of messages expected */
    int m_nExpectedMessageCount;

    /** Invalid interface count<br>
     * The number of ESS messages received on the wrong interface.<br>
     * A non zero value here indicates that an Alarm was received on the Event interface
     * or that an Event was received on the Alarm interface.
     */
    int m_nInvalidInterfaceCount;

    /** Invalid message count.<br>
     * The number of non ESS messages received.<br>
     * A non zero value here indicates a filter failure.
     */
    int m_nInvalidMessageCount;

    /** Invalid ESS message count.<br>
     * The number of ESS messages received that don't belong in the current stream type.
     * A non zero value here indicates a filter setup error.
     */
    int m_nInvalidESSMessageCount;

    /** Valid message count<br>
     * This is the total number of allowed messages received.<br>
     * This value should be really close to the m_nExpectedMessageCount<br>
     * NOTE: Some number of dropped messages are OK.
     */
    int m_nValidMessageCount;

    /** The stream type expected */
    unsigned int m_nStreamType;
};
