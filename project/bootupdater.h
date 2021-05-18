#ifndef BOOTUPDATER_H
#define BOOTUPDATER_H

#include <string>

using namespace std;

class BootUpdater {
public:
    BootUpdater();
    bool switchRoot(string partition);
};

class uBootUpdater: public BootUpdater{
public:
    uBootUpdater();
    bool switchRoot(string partition);
};

class cBootUpdater: public BootUpdater{
public:
    cBootUpdater();
    bool switchRoot(string partition);
};

#endif // BOOTUPDATER_H
