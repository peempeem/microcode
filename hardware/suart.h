#include "../hal/hal.h"
#include "../util/bytestream.h"
#include "../util/mutex.h"
#include <driver/uart.h>

class SUART
{
    public:
        const unsigned MaxBaudrate = 5000000;

        struct BackendData
        {
            uart_port_t port;
            unsigned latency;
            ByteStream rx;
            ByteStream tx;
            Mutex write;
            Mutex read;
        };

        SUART(uart_port_t port=UART_NUM_0) : _port(port) {}
        ~SUART();
    
        bool init(
            unsigned baudrate=115200, 
            unsigned rx=3, 
            unsigned tx=1, 
            bool backend=true, 
            uart_word_length_t data_bits=UART_DATA_8_BITS, 
            uart_parity_t parity=UART_PARITY_DISABLE, 
            uart_stop_bits_t stop_bits=UART_STOP_BITS_1, 
            unsigned latency=1);
        
        bool deinit();

        bool write(uint8_t* data, unsigned len);
        bool write();
        bool writeBypass(uint8_t* data, unsigned len);
        bool read();

        ByteStream* getTXStream() { return (_backend) ? &_backend->tx : NULL; }
        ByteStream* getRXStream() { return (_backend) ? &_backend->rx : NULL; }

    private:
        uart_port_t _port;
        BackendData* _backend = NULL;
        bool _usingTask = false;
        xTaskHandle _task;
};

static SUART suart0(UART_NUM_0);

#if UART_NUM_MAX > 1
static SUART suart1(UART_NUM_1);
#endif

#if UART_NUM_MAX > 2
static SUART suart2(UART_NUM_2);
#endif


