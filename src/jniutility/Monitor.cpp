#include "Monitor.h"
#include <jvmti.h>
#include "../common/Global.h"


USE_NS(NS_COMMON)

AutoMonitor::AutoMonitor(jrawMonitorID monitor)
{
   rawMonitor = monitor;
   Global::jvmti->RawMonitorEnter(rawMonitor);
}

AutoMonitor::~AutoMonitor()
{
	if (rawMonitor != NULL)
		Global::jvmti->RawMonitorExit(rawMonitor);
}
