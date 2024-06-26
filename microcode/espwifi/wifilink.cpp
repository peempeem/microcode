#include "wifilink.h"
#include "../util/mutex.h"
#include "../util/log.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

const static char* LogHeader = "WiFiLink";

class RecvMsg
{
    public:
        MAC mac;
        uint8_t* data;
        unsigned len;

        RecvMsg(const uint8_t* mac, const uint8_t* data, unsigned len) : mac(mac), len(len)
        {
            if (!len)
                data = NULL;
            else
            {
                this->data = new uint8_t[len];
                memcpy(this->data, data, len);
            }
        }

        RecvMsg(const RecvMsg& other)
        {
            copy(other);
        }

        ~RecvMsg()
        {
            destroy();
        }

        void operator=(const RecvMsg& other)
        {
            destroy();
            copy(other);
        }

    private:
        void copy(const RecvMsg& other)
        {
            mac = other.mac;
            len = other.len;
            data = new uint8_t[len];
            memcpy(data, other.data, len);
        }

        void destroy()
        {
            if (data)
                delete[] data;
        }
};

typedef struct
{
    uint16_t frame_head;
    uint16_t duration;
    uint8_t destination_address[6];
    uint8_t source_address[6];
    uint8_t broadcast_address[6];
    uint16_t sequence_control;

    uint8_t category_code;
    uint8_t organization_identifier[3]; // 0x18fe34
    uint8_t random_values[4];
    struct {
        uint8_t element_id;                 // 0xdd
        uint8_t length;                     //
        uint8_t organization_identifier[3]; // 0x18fe34
        uint8_t type;                       // 4
        uint8_t version;
        uint8_t body[0];
    } vendor_specific_content;
} __attribute__ ((packed)) espnow_frame_format_t;

static Mutex recvLock;
static std::queue<RecvMsg> recvMsgs;
static std::queue<int> recvRssi;

void recvCallback(const uint8_t* mac, const uint8_t* data, int len)
{
    recvLock.lock();
    recvMsgs.emplace(mac, data, len);
    wifi_promiscuous_pkt_t* promiscuous_pkt = (wifi_promiscuous_pkt_t*) (data - sizeof(wifi_pkt_rx_ctrl_t) - sizeof(espnow_frame_format_t));
    recvRssi.push(promiscuous_pkt->rx_ctrl.rssi);
    recvLock.unlock();
}

void randomAscii(char* buf, int len)
{
    for (int i = 0; i < len; i++)
        buf[i] = random(65, 91);
}

WiFiLink::WiFiLink()
{

}

bool WiFiLink::init(const char* name, unsigned random)
{
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);

    unsigned len = strlen(name);
    char* nameBuf = new char[len + random + 2];
    strncpy(nameBuf, name, len);
    if (random)
    {
        nameBuf[len++] = ' ';
        randomAscii(&nameBuf[len], random);
        len += random;
    }
    nameBuf[len] = 0;

    char passBuf[16];
    randomAscii(passBuf, 15);
    passBuf[15] = 0;

    bool initialized = WiFi.softAP(nameBuf, passBuf, 1, 0);
    delete[] nameBuf;

    if (!initialized)
        Log(LogHeader) << "WiFi.softAP() failed\n";

    if (esp_now_init() != ESP_OK)
    {
        Log(LogHeader) << "ESP Now init failed\n";
        initialized = false;
    }

    if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR) != ESP_OK)
    {

        Log(LogHeader) << "ESP Now set protocol failed\n";
        initialized = false;
    }

    if (esp_now_register_recv_cb(recvCallback) != ESP_OK)
    {
        Log(LogHeader) << "ESP Now recv callback registration failed\n";
        initialized = false;
    }

    _initialized = initialized;
    return initialized;
}

void WiFiLink::setClockSpeed(unsigned megahertz)
{
    if (!_initialized)
    {
        setCpuFrequencyMhz(megahertz);
        return;
    }

    _scanning = false;

    esp_now_peer_num_t peer_num;
    if (esp_now_get_peer_num(&peer_num) != ESP_OK)
        peer_num.total_num = 0;
        
    esp_now_peer_info_t* peers = new esp_now_peer_info_t[peer_num.total_num];
    for (int i = 0; i < peer_num.total_num; i++)
    {
        esp_now_fetch_peer(true, &peers[i]);
        esp_now_del_peer(peers[i].peer_addr);
    }
    esp_now_deinit();
    esp_wifi_stop();

    setCpuFrequencyMhz(megahertz);

    esp_wifi_start();

    bool initialized = false;

    if (esp_now_init() != ESP_OK)
    {
        Log(LogHeader) << "ESP Now init failed when changing clock speed\n";
        initialized = false;
    }

    if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR) != ESP_OK)
    {
        Log(LogHeader) << "ESP Now set protocol failed when changing clock speed\n";
        initialized = false;
    }

    if (esp_now_register_recv_cb(recvCallback) != ESP_OK)
    {
        Log(LogHeader) << "ESP Now recv callback registration failed when changing clock speed\n";
        initialized = false;
    }

    _initialized = initialized;
    if (!initialized)
        return;
    
    for (int i = 0; i < peer_num.total_num; i++)
        esp_now_add_peer(&peers[i]);
    
    delete[] peers;
}

bool WiFiLink::add(MACLink& maclink)
{
    if (contains(maclink))
        return false;

    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    MAC mac = maclink.getMAC();
    memcpy(&peer.peer_addr, mac.addr, sizeof(MAC));
    peer.channel = 1;
    Log log(LogHeader);
    log << "Connecting to peer with MAC [" << mac.toString().c_str() << "]";

    if (esp_now_add_peer(&peer) != ESP_OK)
    {
        log.failed();
        return false;
    }

    log.success();
    maclinks.push_back(&maclink);
    return true;
}

bool WiFiLink::contains(MACLink& maclink)
{
    std::list<MACLink*>::iterator it;
    return _get(maclink, it);
}

void WiFiLink::remove(MACLink& maclink)
{
    std::list<MACLink*>::iterator it;
    if (_get(maclink, it))
    {
        esp_now_del_peer(maclink.getMAC().addr);
        maclinks.erase(it);
    }
}

void WiFiLink::update()
{
    while (true)
    {
        recvLock.lock();
        bool empty = recvMsgs.empty();
        if (empty)
        {
            recvLock.unlock();
            break;
        }
        
        RecvMsg msg = recvMsgs.front();
        int rssi = recvRssi.front();
        recvMsgs.pop();
        recvRssi.pop();
        recvLock.unlock();

        std::list<MACLink*>::iterator it;
        if (_get(msg.mac, it))
            (**it)._recv_cb(msg.data, msg.len, rssi);
        else if (_pairing && msg.len && (unsigned) msg.data[0] == ReservedMsgs::heartbeat)
        {
            for (MAC& pair : pairings)
                if (pair == msg.mac)
                    return;
            pairings.push_back(msg.mac);
        }
    }

    for (MACLink* maclink : maclinks)
        maclink->update();
    
    int networks = WiFi.scanComplete();
    if (networks >= 0)
    {
        if (_scanning)
        {
            for (unsigned i = 0; i < networks; i++)
            {
                std::string ssid(WiFi.SSID(i).c_str());
                bool substr = ssid.size() >= _key.size();
                for (unsigned i = 0; i < _key.size(); i++)
                {
                    if (!substr)
                        break;
                    substr = _key[i] == ssid[i];
                }
                if (substr)
                {
                    scans.emplace_back(WiFi.BSSID(i));
                    scans.back().addr[sizeof(MAC) - 1] &= 0xfe;
                }
            }
            _scanning = false;
        }
        WiFi.scanDelete();
    }
}

bool WiFiLink::scan(const char* key)
{
    if (_scanning)
        return _scanning;
    
    _key = key;
    
    WiFi.scanNetworks(true);
    scans.clear();
    _scanning = true;
    return _scanning;
}

bool WiFiLink::isScanning()
{
    return _scanning;
}

std::list<MAC> WiFiLink::getScans()
{
    return scans;
}

void WiFiLink::emptyScans()
{
    scans.clear();
}

void WiFiLink::setPairing(bool pairing)
{
    _pairing = pairing;
    if (!pairing)
        pairings.clear();
}

bool WiFiLink::isPairing()
{
    return _pairing;
}

std::list<MAC> WiFiLink::getPairings()
{
    return pairings;
}

bool WiFiLink::_get(MAC& mac, std::list<MACLink*>::iterator& it)
{
    for (it = maclinks.begin(); it != maclinks.end(); it++)
    {
        if (mac == (**it).getMAC())
            return true;
    }
    return false;
}

bool WiFiLink::_get(MACLink& maclink, std::list<MACLink*>::iterator& it)
{
    for (it = maclinks.begin(); it != maclinks.end(); it++)
    {
        if (maclink == **it)
            return true;
    }
    return false;
}
