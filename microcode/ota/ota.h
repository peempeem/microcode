#pragma once

class OTAUpdater
{
    public:
        unsigned init(unsigned port=80);
};

static OTAUpdater ota;