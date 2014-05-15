#include <string>
#include <map>
#include "actioneventqueue.h"
#include "aiplugman.h"
#include "ccmap.h"
#include "dllibrary.h"
#include "logger.h"
#include "message.h"
#include "playerpool.h"
#include "unitandstructurepool.h"
#include "aistubinterface.h"

using namespace AI;

AIStubInterface* curstub;
AIUnit *aiunits0,*aiunits1;

extern "C" void* initAI(const char *options, AIStubInterface *stub) {
   curstub=stub;
   Uint32 numunits0=curstub->getUnit(0,&aiunits0);
   Uint32 numunits1=curstub->getUnitAt(0,2161,5,&aiunits1);
   for (Uint8 i=0; i<numunits0; i++) {
     curstub->giveUnitOrder(&aiunits0[i], NULL);
//       curstub->testUnitOrder(&aiunits0[i], &aiunits1[0]);
   }
//   for (Uint8 i=0; i<numunits1; i++) {
//     curstub->giveUnitOrder(&aiunits1[i], NULL);
//   }
   return NULL;
}

extern "C" void shutdownAI(void *AI) {
}

extern "C" void runAI(void *AI) {
}
