#include "ota.h"
#include "../util/log.h"
#include "../util/pvar.h"
#include "../util/timer.h"
#include "../util/mutex.h"
#include <WebServer.h>
#include <Update.h>

const char* LOG_HEADER = "OTA";
const char* safeModeFile = "/_safemode.bin";
const char* rootHTML = "<html><form method='POST' action='/ota/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form></html>";
const char* updateHTML = "<html><p>update successful</p><form action='/ota/'><input type='submit' value='return'/></form></html>";
static WebServer server;
static Mutex lock;
static volatile int updateSize = -1;
static volatile bool running = false;
static volatile bool safeMode = false;
static volatile bool safeModeTriggered = false;
static volatile bool rebooting = false;
static volatile uint64_t rebootTime;

PACK(struct SafeMode
{
    unsigned bootAttempts = 0;
    unsigned bootSuccesses = 0;
    unsigned bootDifferential = 2;
    unsigned bootTimeout = 20e3;
});

static PVar<SafeMode> smvar(safeModeFile);

// backend server updater thread
void update(void* args)
{
    uint32_t notification = 0;
    Timer t(smvar.data.bootTimeout);
    bool bootSuccess = false;
    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 20 / portTICK_PERIOD_MS);
        server.handleClient();

        lock.lock();
        if (safeMode && !bootSuccess && t.isRinging())
        {
            smvar.data.bootSuccesses++;
            smvar.save();
            bootSuccess = true;
        }

        if (rebooting && sysTime() >= rebootTime)
            esp_restart();
        lock.unlock();

        // kill task if notified
        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}

void onRoot()
{
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", rootHTML);
}

void onUpdate()
{
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", updateHTML);
}

void onUpdateUpdater()
{
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        Update.begin();
        Log(LOG_HEADER) << "Update begin";
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        lock.lock();
        updateSize = upload.totalSize;
        lock.unlock();
        Update.write(upload.buf, upload.currentSize);
        Log(LOG_HEADER) << "Update size: " << updateSize;
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        Update.end(true);
        lock.lock();
        if (safeMode)
        {
            smvar.data = SafeMode();
            smvar.save();  
        }
        updateSize = -1;
        rebootTime = sysTime() + (unsigned) 1e6;
        rebooting = true;
        lock.unlock();
        Log(LOG_HEADER) << "Update complete. Rebooting shortly ...";
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        lock.lock();
        updateSize = -1;
        lock.unlock();
        Log(LOG_HEADER) << "Update aborted";
    }
}

void onUpdateSize()
{
    server.sendHeader("Connection", "close");
    std::stringstream ss;
    ss << updateSize;
    server.send(200, "text/plain", ss.str().c_str());
}

unsigned OTAUpdater::init(unsigned port, bool safemode)
{
    lock.lock();
    if (running)
    {
        lock.unlock();
        return port;
    }
        
    running = true;
    safeMode = safemode;

    Log(LOG_HEADER) << "Starting Update Server ...";

    // disable no client delay
    server.enableDelay(false);
    
    // add special server functions
    server.on("/ota/", HTTP_GET, onRoot);
    server.on("/ota/update", HTTP_POST, onUpdate, onUpdateUpdater);
    server.on("/ota/updateSize", HTTP_GET, onUpdateSize);

    // start server
    server.begin();
    xTaskCreateUniversal(update, "ota_backend", 4 * 1024, NULL, 20, NULL, 0);

    Log(LOG_HEADER) << "Update Server started";

    if (safeMode)
    {
        if (!filesys.isInitialized() && !filesys.init())
        {
            Log(LOG_HEADER) << "Filesystem failed to initialize -> safemode boot disabled";
            safeMode = false;
            return port;
        }

        smvar.load();

        smvar.data.bootAttempts++;
        if (smvar.data.bootAttempts > smvar.data.bootSuccesses + smvar.data.bootDifferential)
        {
            Log(LOG_HEADER) << "Entering safemode blocking procedure. Waiting for update ...";
            Log(LOG_HEADER) << "boot attempts: " << smvar.data.bootAttempts << " boot successes: " << smvar.data.bootSuccesses;

            safeModeTriggered = true;
            
            while (true)
            {
                lock.unlock();
                Timer t(smvar.data.bootTimeout);
                while (!t.isRinging())
                    sysSleep(5);
                
                lock.lock();

                if (!isRebooting() && !isUpdating())
                {
                    Log(LOG_HEADER) << "No update received. Proceeding with normal boot";
                    filesys.remove(safeModeFile);
                    safeModeTriggered = false;
                    break;
                }
            }
        }
        else
            smvar.save(); 
    }

    lock.unlock();
    return port;
}

bool OTAUpdater::isUpdating()
{
    return updateSize != -1;
}

bool OTAUpdater::isRebooting()
{
    return rebooting;
}

bool OTAUpdater::isInSafeMode()
{
    return safeModeTriggered;
}