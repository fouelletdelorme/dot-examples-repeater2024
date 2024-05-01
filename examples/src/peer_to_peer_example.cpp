#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == PEER_TO_PEER_EXAMPLE

/////////////////////////////////////////////////////////////////////////////
// -------------------- DOT LIBRARY REQUIRED ------------------------------//
// * Because these example programs can be used for both mDot and xDot     //
//     devices, the LoRa stack is not included. The libmDot library should //
//     be imported if building for mDot devices. The libxDot library       //
//     should be imported if building for xDot devices.                    //
// * https://developer.mbed.org/teams/MultiTech/code/libmDot-dev-mbed5/    //
// * https://developer.mbed.org/teams/MultiTech/code/libmDot-mbed5/        //
// * https://developer.mbed.org/teams/MultiTech/code/libxDot-dev-mbed5/    //
// * https://developer.mbed.org/teams/MultiTech/code/libxDot-mbed5/        //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// * these options must match between the two devices in   //
//   order for communication to be successful
/////////////////////////////////////////////////////////////
static uint8_t network_address[] = { 0x01, 0x02, 0x03, 0x04 };
static uint8_t network_session_key[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
static uint8_t data_session_key[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
uint32_t tx_frequency = 915500000;
uint8_t tx_datarate = lora::DR_13;
uint8_t tx_power = 20;

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX, 115200);
mbed::BufferedSerial serial_AT(PA_2, PA_3, 115200);   // AT serial port

AnalogIn lux(XBEE_AD0);

int main() {
    // Custom event handler for automatically displaying RX data
    RadioEvent events;

    // pc.baud(115200);
    // serial_AT.baud(115200);

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);
    dot = mDot::getInstance(plan);
    assert(dot);

    logInfo("mbed-os library version: %d.%d.%d", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    // start from a well-known state
    logInfo("defaulting Dot configuration");
    dot->resetConfig();

    // make sure library logging is turned on
    dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

    // attach the custom events handler
    dot->setEvents(&events);

    // Enable FOTA for multicast support
    Fota::getInstance(dot);

    // update configuration if necessary
    if (dot->getJoinMode() != mDot::PEER_TO_PEER) {
        logInfo("changing network join mode to PEER_TO_PEER");
        if (dot->setJoinMode(mDot::PEER_TO_PEER) != mDot::MDOT_OK) {
            logError("failed to set network join mode to PEER_TO_PEER");
        }
    }

    uint8_t frequency_band = dot->getFrequencyBand();

    // in PEER_TO_PEER mode there is no join request/response transaction
    // as long as both Dots are configured correctly, they should be able to communicate
    update_peer_to_peer_config(network_address, network_session_key, data_session_key, tx_frequency, tx_datarate, tx_power);

    // save changes to configuration
    logInfo("saving configuration");
    if (!dot->saveConfig()) {
        logError("failed to save configuration");
    }

    // display configuration
    display_config();

    char buffer[1024];
    while (true) {

        // join network if not joined
        if (!dot->getNetworkJoinStatus()) {
            join_network();
        }

        printf("\r\nREADING FROM PC...\r\n");
        memset(buffer, 0, sizeof(buffer));
        // char buffer[1024];
        scanf("%s", buffer);
        // printf("got:\r\n");
        printf("<< %s\r\n", buffer);
        printf("strlen = %d\r\n", strlen(buffer));
        printf("sizeof = %d\r\n", sizeof(buffer));
        printf("DONE READING FROM PC\r\n\r\n");

        printf("SENDING TO AT:\r\n>> %s\r\n", buffer);
        buffer[strlen(buffer)] = '\r';
        buffer[strlen(buffer)] = '\n';
        printf("new strlen = %d\r\n", strlen(buffer));
        serial_AT.write(buffer, strlen(buffer));
        printf("DONE SENDING TO AT\r\n\r\n");

        wait_us(8000000);

        printf("READING FROM AT...\r\n");
        char received;
        string receivedstring = "";
        while(serial_AT.readable())
        {
            serial_AT.read(&received, 1); // guaranteed to return because the port is readable
            receivedstring += received;
            // if(received == '\r')
            // {
            //     receivedstring += '\n';
            // }
        }
        printf("<< %s\r\n", receivedstring.c_str());
        printf("DONE READING FROM AT\r\n\r\n");

        // std::string response = ParseATResponse();
    
        // the Dot can't sleep in PEER_TO_PEER mode; it must be waiting for data from the other Dot
        logInfo("... sleeping for 2s ...");
        ThisThread::sleep_for(2s);
    }

    return 0;
}

#endif
