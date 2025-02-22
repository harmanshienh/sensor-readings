#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringSerial.h>

int main() {
    int serial_port;
    float temp, humidity;
    char buffer[sizeof(float)];

    //Open UART serial port
    if ((serial_port = serialOpen("/dev/serial0", 9600)) < 0) {
        fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
        return 1;
    }

    //Initialize wiringPi
    if (wiringPiSetup() == -1) {
        fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
        return 1;
    }

    while (1) {
        //Check for incoming data
        if (serialDataAvail(serial_port) >= sizeof(float)) {
            //Extract temperature
            for (int i = 0; i < sizeof(float); ++i) {
                buffer[i] = serialGetchar(serial_port);
            }
            memcpy(&temp, buffer, sizeof(float));

            //Extract humidity
            for (int i = 0; i < sizeof(float); ++i) {
                buffer[i] = serialGetchar(serial_port);
            }
            memcpy(&humidity, buffer, sizeof(float));

            printf("Temperature %.2f°C, Humidity: %.2f%%\n", temp, humidity);
            fflush(stdout);
        }
    }
}