void setup()
{
    Serial.begin(500000);
    Serial2.begin(500000, SERIAL_8N1, 35, 32);
}

void loop()
{
    if (Serial.available())
        Serial2.write(Serial.read());
    if (Serial2.available())
        Serial.write(Serial2.read());
}
