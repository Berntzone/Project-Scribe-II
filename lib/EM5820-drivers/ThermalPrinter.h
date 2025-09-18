#ifndef THERMAL_PRINTER_H
#define THERMAL_PRINTER_H

#include <Arduino.h>

class ThermalPrinter {
  public:
    ThermalPrinter(HardwareSerial &serialPort = Serial);

    void begin(uint32_t baud = 9600);

    void init();                                        // ESC @ reset printer
    void setHeat(uint8_t heat);                         // ESC ## set print heat
    void setSpeed(uint8_t speed);                       // ESC ## set print speed
    void setCodePage(uint8_t n);                        // ESC t n (0=PC437, 1=Katakana, 2=PC850, 3=PC860, 4=PC863, 5=PC865, 6=West Europe, 7=Greek, 8=Hebrew, 9=East Europe, 10=Iran, 11=WPC1252)
    void setOrientationUpsideDown(bool on);             // ESC { n (0=normal, 1=180° rotated) 
    void setAlign(uint8_t n);                           // ESC a n (0=left, 1=center, 2=right)
    void setBold(bool on);                              // ESC E n
    void setUnderline(uint8_t n);                       // ESC - n (0=off, 1=thin, 2=thick)
    void setInverse(bool on);         // GS B n (true = on, false = off)
    void setCharSize(uint8_t width, uint8_t height);    // GS ! n (width: 0-7, height: 0-7)
    void setFont(uint8_t n);                            // ESC M n (0=12x24, 1=9x17)
    void feed(uint8_t n = 1);                           // ESC d n (advance paper n lines)
    void write(uint8_t c);                              // write a single byte/character
    void print(String txt);                             // normal text
    void println(String txt);                           // normal text with newline
    void printInverted(String txt);                     // Inverted text with newline without white spaces, needs code page 0 (PC437) for block character
    void printWrappedUpsideDown(String text);           // print wrapped text upside down

    // Graphics / QR wrappers
    void printBitmap(uint16_t width, uint16_t height, const uint8_t *data);
    void printQRCode(const char *data);

    void printCodePages();                              // Print a test page of all code pages

  private:
    HardwareSerial *printer;
};

#endif
