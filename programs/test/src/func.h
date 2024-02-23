#pragma once

#include "types.h"

float adcVoltageToPressure(float voltage)
{
    return 20 + 230 * ((voltage - 0.204f) / 4.692f);
}

void quaternionToEuler(float qr, float qi, float qj, float qk, Eular* ypr, bool degrees=false)
{
    float sqr = sq(qr);
    float sqi = sq(qi);
    float sqj = sq(qj);
    float sqk = sq(qk);

    ypr->yaw = atan2f(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));
    ypr->pitch = asinf(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));
    ypr->roll = atan2f(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));

    if (degrees) 
    {
        ypr->yaw *= RAD_TO_DEG;
        ypr->pitch *= RAD_TO_DEG;
        ypr->roll *= RAD_TO_DEG;
    }
}