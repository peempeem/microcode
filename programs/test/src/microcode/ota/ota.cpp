#include "ota.h"
#include "../util/log.h"
#include <WebServer.h>
#include <Update.h>

const char* rootHTML = "<html><form method='POST' action='/ota/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form></html>";
const char* updateHTML = "<html><p>update successful</p><form action='/ota/'><input type='submit' value='return'/></form></html>";
WebServer server;
int updateSize = -1;
bool running = false;

// reboots in 1 second unless aborted through task notification
void reboot(void* args)
{
    uint32_t notification = 0;
    xTaskNotifyWait(0, UINT32_MAX, &notification, 1000 / portTICK_PERIOD_MS);
    
    if (notification & BIT(0))
        vTaskDelete(NULL);
    ESP.restart();
}

// backend server updater thread
void update(void* args)
{
    uint32_t notification = 0;
    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1 / portTICK_PERIOD_MS);
        server.handleClient();

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
        Log("OTA") << "Update begin.";
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        updateSize = upload.totalSize;
        Update.write(upload.buf, upload.currentSize);
        Log("OTA") << "Update size: " << updateSize;
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        xTaskCreateUniversal(reboot, "reboot", 1024, NULL, configMAX_PRIORITIES, NULL, tskNO_AFFINITY);
        Update.end(true);
        Log("OTA") << "Update complete. Rebooting shortly...";
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        updateSize = -1;
        Log("OTA") << "Update aborted.";
    }
}

void onUpdateSize()
{
    server.sendHeader("Connection", "close");
    std::stringstream ss;
    ss << updateSize;
    server.send(200, "text/plain", ss.str().c_str());
}

unsigned OTAUpdater::init(unsigned port)
{
    if (running)
        return port;
    running = true;

    // disable no client delay
    server.enableDelay(false);
    
    // add special server functions
    server.on("/ota/", HTTP_GET, onRoot);
    server.on("/ota/update", HTTP_POST, onUpdate, onUpdateUpdater);
    server.on("/ota/updateSize", HTTP_GET, onUpdateSize);

    // start server
    server.begin();
    xTaskCreateUniversal(update, "ota_backend", 1024 * 3, NULL, configMAX_PRIORITIES - 2, NULL, tskNO_AFFINITY);

    return port;
}