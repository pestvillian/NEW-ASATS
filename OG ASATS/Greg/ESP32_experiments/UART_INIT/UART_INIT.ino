#define RXD2 18  // UART1 RX Pin
#define TXD2 17  // UART1 TX Pin

void setup() {
    Serial.begin(115200);   // USB Serial
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART1 Configuration
}

void loop() {
    Serial2.println("Hello, UART2!");
    delay(1000);
}
