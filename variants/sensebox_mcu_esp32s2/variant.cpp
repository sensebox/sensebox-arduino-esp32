#include "esp32-hal-gpio.h"
#include "pins_arduino.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_log.h"

// Globale Variable zur Kommunikation mit dem Hauptprogramm
bool buttonWasPressed = false;

extern "C" {

// Initialize variant/board, called before setup()
void initVariant(void) {
    // Initialisiere Pins wie vorher
    pinMode(IO_ENABLE, OUTPUT);
    digitalWrite(IO_ENABLE, LOW);

    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
     
    pinMode(PIN_XB1_ENABLE, OUTPUT);
    digitalWrite(PIN_XB1_ENABLE, LOW);
 
    pinMode(PIN_UART_ENABLE, OUTPUT);
    digitalWrite(PIN_UART_ENABLE, LOW);

    pinMode(PD_ENABLE, OUTPUT);
    digitalWrite(PD_ENABLE, HIGH);

    // Neuen Button-Pin definieren (z.B. GPIO 0 als Beispiel)
    const int PIN_BUTTON = 0;
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // Button gedrückt halten
    unsigned long pressStartTime = 0;
    bool buttonPressed = false;

    // Warten auf Button-Eingabe für 5 Sekunden
    unsigned long startTime = millis();

    // Überprüfen, ob der Button gedrückt wird
    while (millis() - startTime < 5000) {
        if (digitalRead(PIN_BUTTON) == LOW) {
            if (!buttonPressed) {
                // Der Button wurde gerade gedrückt
                buttonPressed = true;
            }
        } else if (buttonPressed) {
            // Wenn der Button gedrückt und dann losgelassen wird, in OTA1-Partition booten
            const esp_partition_t* ota1_partition = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

            if (ota1_partition) {
                esp_err_t err = esp_ota_set_boot_partition(ota1_partition);
                if (err == ESP_OK) {
                    esp_restart();  // Neustarten, um die OTA1-Partition zu booten
                } else {
                    // Fehler beim Setzen der Boot-Partition
                    ESP_LOGE("OTA", "Fehler beim Setzen der OTA1-Partition: %s", esp_err_to_name(err));
                }
            }
            // Nach dem Loslassen des Buttons abbrechen
            break;
        }
    }
}

}
