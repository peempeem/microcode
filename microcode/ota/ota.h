#pragma once

class OTAUpdater
{
    public:
        unsigned init(unsigned port=80, bool safemode=true);
        bool isUpdating();
        bool isRebooting();
};

static OTAUpdater ota;