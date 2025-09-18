#include "ThermalPrinter.h"
#define maxCharsPerLine 32


ThermalPrinter::ThermalPrinter(HardwareSerial &serialPort) {
  printer = &serialPort;
}

void ThermalPrinter::begin(uint32_t baud) {
  printer->begin(baud);
  delay(100);
}

void ThermalPrinter::init() {
  printer->write(0x1B); printer->write('@');                                // ESC @ initialize printer
}

void ThermalPrinter::setHeat(uint8_t heat) { // ESC ## STDP heat
  byte cmd[] = {0x1B, 0x23, 0x23, 0x53, 0x54, 0x44, 0x50, heat};
  printer->write(cmd, sizeof(cmd));                                         // (0-9, 0=light, 9=dark)
}

void ThermalPrinter::setSpeed(uint8_t speed) { // ESC ## STSP speed
  byte cmd[] = {0x1B, 0x23, 0x23, 0x53, 0x54, 0x53, 0x50, speed};
  printer->write(cmd, sizeof(cmd));                                         // (0-9, 0=slow, 9=fast)
}

void ThermalPrinter::setCodePage(uint8_t n) {
  // printer->write(0x1B); printer->write('t'); printer->write(n);          // (0=PC437, 1=Katakana, 2=PC850, 3=PC860, 4=PC863, 5=PC865, 6=West Europe, 7=Greek, 8=Hebrew, 9=East Europe, 10=Iran, 11=WPC1252)
  byte cmd[] = {0x1B, 0x23, 0x23, 0x53, 0x4C, 0x41, 0x4E, n};
  printer->write(cmd, sizeof(cmd));
}

void ThermalPrinter::setOrientationUpsideDown(bool on) {
  printer->write(0x1B); printer->write('{'); printer->write(on ? 1 : 0);    // (0=normal, 1=180° rotated)
}

void ThermalPrinter::setAlign(uint8_t n) {
  printer->write(0x1B); printer->write('a'); printer->write(n);             // (0=left, 1=center, 2=right)
}

void ThermalPrinter::setBold(bool on) {
  printer->write(0x1B); printer->write('E'); printer->write(on ? 1 : 0);    // (true = on, false = off)
}

void ThermalPrinter::setUnderline(uint8_t n) {
  printer->write(0x1B); printer->write('-'); printer->write(n);             // (0=off, 1=thin, 2=thick)
}

void ThermalPrinter::setInverse(bool on) {
  byte cmd[] = {0x1D, 0x42};
  printer->write(cmd, sizeof(cmd));
  printer->write(on ? 1 : 0);
  // printer->write(0x1D); printer->write(0x42); printer->write(on ? 1 : 0);   // (true = on, false = off)
}

void ThermalPrinter::setCharSize(uint8_t width, uint8_t height) {
  uint8_t size = ((width & 0x07) << 4) | (height & 0x07);                   // width: 0-7, height: 0-7
  printer->write(0x1D); printer->write('!'); printer->write(size);         
}

void ThermalPrinter::setFont(uint8_t n) {
  printer->write(0x1B); printer->write('M'); printer->write(n);             // (0=12x24, 1=9x17)
}

void ThermalPrinter::feed(uint8_t n) {
  printer->write(0x1B); printer->write('d'); printer->write(n);             // (advance n lines)
}

void ThermalPrinter::write(uint8_t c) {
  printer->write(c);
}

void ThermalPrinter::print(String txt) {                                    // normal text
  printer->print(txt);
}

void ThermalPrinter::println(String txt) {                                  // normal text with newline
  printer->println(txt);
}

void ThermalPrinter::printInverted(String txt) {                            // inverted text with newline
  // --- OPTION 1: Simple inversion (needs PC437)
  
  printer->write(0xDB);  // full block character in PC437
  setInverse(true);
  for (int i = 0; txt[i]; i++) {
    if (txt[i] == ' ') {
      setInverse(false);
      printer->write(0xDB);  // full block character in PC437
      setInverse(true);
    } else {
      printer->write(txt[i]);
    }
  }
  setInverse(false);
  printer->write(0xDB);  // full block character in PC437

  // OPTION 2: Print graphical black block

  // byte blackBlock[] = {
  //   0x1D, 0x76, 0x30, 0x00, // GS v 0, normal
  //   0x01, 0x00,             // width = 1 byte
  //   0x01, 0x00,             // height = 1 byte
  //   0xFF                    // 8 black pixels
  // };

  // setInverse(true);
  // for (int i = 0; txt[i]; i++) {
  //   if (txt[i] == ' ') {
  //     printer->write(blackBlock, sizeof(blackBlock));
  //   } else {
  //     printer->write(txt[i]);
  //   }
  // }
  // setInverse(false);
  
}

void ThermalPrinter::printWrappedUpsideDown(String text) {                  // print wrapped text upside down
  String lines[100];
  int lineCount = 0;
  
  while (text.length() > 0) {
    // int breakIndex = maxCharsPerLine;
    if (text.length() <= maxCharsPerLine) {
      lines[lineCount++] = text;
      break;
    }
    
    int lastSpace = text.lastIndexOf(' ', maxCharsPerLine);
    if (lastSpace == -1) lastSpace = maxCharsPerLine;
    
    lines[lineCount++] = text.substring(0, lastSpace);
    text = text.substring(lastSpace);
    text.trim();
  }
  
  for (int i = lineCount - 1; i >= 0; i--) {
    printer->println(lines[i]);
  }
}

void ThermalPrinter::printBitmap(uint16_t width, uint16_t height, const uint8_t *data) {
  // Use GS v 0 m xL xH yL yH d1...dk
  uint8_t xL = width & 0xFF;
  uint8_t xH = (width >> 8) & 0xFF;
  uint8_t yL = height & 0xFF;
  uint8_t yH = (height >> 8) & 0xFF;

  printer->write(0x1D); printer->write('v'); printer->write('0'); printer->write(0); // m=0 normal
  printer->write(xL); printer->write(xH);
  printer->write(yL); printer->write(yH);

  size_t len = (size_t)width * height;
  printer->write(data, len);
}

void ThermalPrinter::printQRCode(const char *data) {
  // Simplified QR example (depends on printer firmware, ESC/POS standard)
  // Store QR code data in buffer
  uint16_t len = strlen(data);

  // [Step 1] Store data
  printer->write(0x1D); printer->write('('); printer->write('k');
  printer->write((len + 3) & 0xFF);
  printer->write(((len + 3) >> 8) & 0xFF);
  printer->write(49); printer->write(80); printer->write(48);
  printer->write((const uint8_t*)data, len);

  // [Step 2] Print QR
  printer->write(0x1D); printer->write('('); printer->write('k');
  printer->write(3); printer->write(0);
  printer->write(49); printer->write(81); printer->write(48);
}

void ThermalPrinter::printCodePages() {
  init();

  // Loop through first 10 code pages
  for (int n = 0; n < 10; n++) {
    // Select code page n
    printer->write(0x1B);
    printer->write(0x74);
    printer->write(n);

    // Print header
    print("Code page ");
    println((String) n);

    // Print all 256 characters
    for (int c = 0; c < 256; c++) {
      printer->write(c);
      if ((c + 1) % 32 == 0) Serial.write('\n');
    }

    feed(2);
    delay(2000); // pause between code pages
  }
}