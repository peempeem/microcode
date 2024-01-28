#include "suart.h"
#include "../util/log.h"

const static char* LogHeader = "SUART";

#define BUF_SLICE 64

void send_bytes(SUART::BackendData* backend)
{
    size_t bufSize;
    uint8_t buf[BUF_SLICE];
    backend->write.lock();
    uart_get_tx_buffer_free_size(backend->port, &bufSize); 
    while (bufSize)
    {
        unsigned maxPull = min((int) bufSize, BUF_SLICE);
        unsigned pulled = backend->tx.get(buf, maxPull);

        if (!pulled)
            break;

        uart_write_bytes(backend->port, buf, pulled);

        if (pulled < maxPull)
            break;

        uart_get_tx_buffer_free_size(backend->port, &bufSize);
        sizeof(SUART);
    }
    backend->write.unlock();
}

void recv_bytes(SUART::BackendData* backend)
{
    size_t bufSize;
    uint8_t buf[BUF_SLICE];
    backend->read.lock();
    uart_get_buffered_data_len(backend->port, &bufSize);
    while (bufSize)
    {
        unsigned pulled = uart_read_bytes(backend->port, buf, min((int) bufSize, BUF_SLICE), 0);
        backend->rx.put(buf, pulled);
        uart_get_buffered_data_len(backend->port, &bufSize);
    }
    backend->read.unlock();
}

void SUARTBackend(void* args)
{
    SUART::BackendData* backend = (SUART::BackendData*) args;

    uint32_t notification = 0;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, backend->latency / portTICK_PERIOD_MS);

        send_bytes(backend);
        recv_bytes(backend);

        if (notification & BIT(0))
        {
            delete backend;
            vTaskDelete(NULL);
        }
    }
}

SUART::~SUART()
{
    if (_backend)
        deinit();
}

bool SUART::init(
    unsigned baudrate, 
    unsigned rx, 
    unsigned tx, 
    uart_word_length_t dataBits, 
    uart_parity_t parity, 
    uart_stop_bits_t stopBits,
    unsigned latency)
{

    if (_backend)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because already initialized";
        return false;
    }
    
    if (baudrate > MaxBaudrate)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because baudrate parameter [" << baudrate << "] exceeds maximum allowed value [" << MaxBaudrate << "]";
        return false;
    }

    unsigned bufSize = max(256, (int) (1.25f * latency * baudrate / ((int) (dataBits + 5) * 1000.0f)));
    
    if (uart_driver_install(_port, bufSize, bufSize, 0, NULL, 0) != ESP_OK)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because driver install failed";
        return false;
    }

    if (uart_set_baudrate(_port, baudrate) != ESP_OK)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because baudrate configuration error";
        uart_driver_delete(_port);
        return false;
    }

    if (uart_set_word_length(_port, dataBits) != ESP_OK)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because word length configuration error";
        uart_driver_delete(_port);
        return false;
    }

    if (uart_set_parity(_port, parity) != ESP_OK)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because pairity configuration error";
        uart_driver_delete(_port);
        return false;
    }
    
    if (uart_set_stop_bits(_port, stopBits) != ESP_OK)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because stop bit configuration error";
        uart_driver_delete(_port);
        return false;
    }

    if (uart_set_pin(_port, tx, rx, -1, -1) != ESP_OK)
    {
        Log(LogHeader) << "Could not initialize SUART [" << _port << "] because pin configuration error";
        uart_driver_delete(_port);
        return false;
    }

    _backend = new BackendData();
    _backend->port = _port;
    _backend->latency = latency;

    xTaskCreateUniversal(SUARTBackend, "suartbackend", 3 * 1024, (void*) _backend, configMAX_PRIORITIES - 1, &_task, tskNO_AFFINITY);

    // Log log(LogHeader);
    // log << "Initialized SUART [" << _port << "]\n";
    // log << "Parameters:\n";
    // log << "Baudrate: " << baudrate << " bits/sec\n";
    // log << "RX: " << rx << " TX: " << tx << "\n";
    // log << "Word Size: " << dataBits + 5 << " bits\n";
    // log << "Pairity: ";
    // switch (parity)
    // {        
    //     case UART_PARITY_EVEN:
    //         log << "Even\n";
    //         break;
        
    //     case UART_PARITY_ODD:
    //         log << "Odd\n";
    //         break;
        
    //     default:
    //         log << "None\n";
    //         break;

    // }
    // log << "Stop Bits: ";
    // switch (stopBits)
    // {
    //     case UART_STOP_BITS_1_5:
    //         log << "1.5 bits\n";
    //         break;
        
    //     case UART_STOP_BITS_2:
    //         log << "2 bits\n";
    //         break;
        
    //     default:
    //         log << "1 bit\n";
    //         break;
    // }
    // log << "Backend Buffer Size: " << bufSize << " bytes\n";
    // log << "Background Task Latency: " << latency << " ms";

    return true;
}

bool SUART::deinit()
{
    if (!_backend)
    {
        Log(LogHeader) << "Could not deinitialize SUART [" << _port << "] because not initialized";
        return false;
    }

    xTaskNotify(_task, BIT(0), eSetBits);
    _backend = NULL;

    if (uart_driver_delete(_port) != ESP_OK)
    {
        Log(LogHeader) << "Could not deinitialize SUART [" << _port << "] because driver removal failed";
        return false;
    }

    // Log(LogHeader) << "Deinitialized SUART [" << _port << "]";

    return true;
}

bool SUART::write(uint8_t* data, unsigned len)
{
    if (!_backend)
        return false;
    _backend->tx.put(data, len);
    send_bytes(_backend);
    return true;
}

bool SUART::read()
{
    if (!_backend)
        return false;
    recv_bytes(_backend);
    return true;
}