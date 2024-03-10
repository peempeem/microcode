#pragma once

#include <stdint.h>

// Pins

#define MCP_CS  22
#define MCP_RST 27

#define ADC_CS      15
#define ADC_DRDY    39

#define IMU_CS  5
#define IMU_INT 25
#define IMU_RST 14

#define PXL_PIN 21
#define NUM_PXL 4

#define RX0 34
#define TX0 33
#define RX1 35
#define TX1 32

// Other Defines

#define ADC_REF 5
#define ADC_PORTS 4
#define IMU_REPORT 5000
#define PIXEL_REFERESH_PERIOD (1000 / 60)
#define DEBUG_BAUD  115200
#define COMM_BAUD (unsigned) 2e6
uint8_t driverMap[] = { 1, 0, 3, 2, 5, 4, 7, 6 };


