#include <FlexCAN_T4.h>

int sendTelemetry(uint32_t, uint64_t*);
int checkCommand(const CAN_message_t);
void printFrame(const CAN_message_t &msg);


