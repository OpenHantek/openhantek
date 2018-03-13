#include "bulkcommand.h"

BulkCommand::BulkCommand(HantekE::BulkCode code, unsigned size): std::vector<uint8_t>(size), code(code) {}
