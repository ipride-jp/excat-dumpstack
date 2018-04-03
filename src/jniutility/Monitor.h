#if !defined(_MONITOR_H)
#define _MONITOR_H

struct _jrawMonitorID;

class AutoMonitor
{
public :
    AutoMonitor(struct _jrawMonitorID* monitor);
    virtual ~AutoMonitor ();
private:
     struct _jrawMonitorID *rawMonitor;
} ;


#endif
