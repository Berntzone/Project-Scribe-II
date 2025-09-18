#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ThermalPrinter.h>     // New printer driver for EM5820 Thermal Printer
#include <WIFI_credentials.h>   // Put your SSID and PASSWORD in this file


// === Printer Configuration ===
#define CODEPAGE 0  // Code page PC427 for printer, US and Europe (needed for Full Block character)
#define HEAT 15     // Printer heat (0-15), higher values print darker
#define SPEED 3     // Print speed (0-9), higher values print faster
#define maxCharsPerLine 32 // Max characters per line for the printer

// === Function Declarations ===
void connectToWiFi();
void setupWebServer();
void handleRoot();
void handleSubmit();
void handle404();
String getFormattedDateTime();
String formatCustomDate(String customDate);
void initializePrinter();
void printReceipt();
void printServerInfo();

// === WiFi Configuration ===
const char* ssid = SSID;
const char* password = PASSWORD;

// === Time Configuration ===
const long utcOffsetInSeconds = 3600; // UTC offset in seconds (0 for UTC, 3600 for UTC+1, etc.)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

// === Web Server ===
ESP8266WebServer server(80);

// === Printer Setup ===
ThermalPrinter printer(Serial);


// === Storage for form data ===
struct Receipt {
  String message;
  String timestamp;
  bool hasData;
};

Receipt currentReceipt = {"", "", false};

void setup() {
  // Initialize printer
  initializePrinter();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize time client
  timeClient.begin();
  printer.println("Time client initialized");
  
  // Setup web server routes
  setupWebServer();
  
  // Start the server
  server.begin();
  printer.println("Web server starting...");
  
  // Print server info
  printServerInfo();

}

void loop() {
  // Handle web server requests
  server.handleClient();
  
  // Update time client
  timeClient.update();
  
  // Check if we have a new receipt to print
  if (currentReceipt.hasData) {
    printReceipt();
    currentReceipt.hasData = false; // Reset flag
  }
  
  delay(1000); // Small delay to prevent excessive CPU usage (OG value: 10ms)
}

// === WiFi Connection ===
void connectToWiFi() {
  // Serial.print("Connecting to WiFi: ");
  // Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    // Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    printer.feed(1);
    printer.println("WiFi connected successfully!");
    printer.print("IP address: ");
    printer.println(WiFi.localIP().toString());
  } else {
    printer.feed(1);
    printer.println("Failed to connect to WiFi");
  }
}

// === Web Server Setup ===
void setupWebServer() {
  // Serve the main page
  server.on("/", HTTP_GET, handleRoot);
  
  // Handle form submission
  server.on("/submit", HTTP_POST, handleSubmit);

  // ADD THIS LINE to also handle submission via URL
  server.on("/submit", HTTP_GET, handleSubmit);
  
  // Handle 404
  server.onNotFound(handle404);
}

// === Web Server Handlers ===
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en" class="bg-gray-50 text-gray-900">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Life Receipt</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <script src="https://cdn.jsdelivr.net/npm/canvas-confetti@1.6.0/dist/confetti.browser.min.js"></script>
  <script defer>
    function handleInput(el) {
      const counter = document.getElementById('char-counter');
      const remaining = 200 - el.value.length;
      counter.textContent = `${remaining} characters left`;
      counter.classList.toggle('text-red-500', remaining <= 20);
    }
    function handleSubmit(e) {
      e.preventDefault();
      const formData = new FormData(e.target);
      fetch('/submit', {
        method: 'POST',
        body: formData
      }).then(() => {
        const form = document.getElementById('receipt-form');
        const message = document.getElementById('thank-you');
        form.classList.add('hidden');
        message.classList.remove('hidden');
        confetti({
          particleCount: 100,
          spread: 70,
          origin: { y: 0.6 },
        });
      });
    }
  </script>
</head>
<body class="flex flex-col min-h-screen justify-between items-center py-12 px-4 font-sans">
  <main class="w-full max-w-md text-center">
    <h1 class="text-3xl font-semibold mb-10 text-gray-900 tracking-tight">Life Receipt:</h1>
    <form id="receipt-form" onsubmit="handleSubmit(event)" action="/submit" method="post" class="bg-white shadow-2xl rounded-3xl p-8 space-y-6 border border-gray-100">
      <textarea
        name="message"
        maxlength="200"
        oninput="handleInput(this)"
        placeholder="Type your receipt…"
        class="w-full p-4 rounded-xl border border-gray-200 focus:outline-none focus:ring-2 focus:ring-gray-400 focus:border-transparent resize-none text-gray-800 placeholder-gray-400"
        rows="4"
        required
        autofocus
      ></textarea>
      <div id="char-counter" class="text-sm text-gray-500 text-right">200 characters left</div>
      <button type="submit" class="w-full bg-gray-900 hover:bg-gray-800 text-white py-3 rounded-xl font-medium transition-all duration-200 hover:scale-[1.02] hover:shadow-lg">
        Send
      </button>
    </form>
    <div id="thank-you" class="hidden text-gray-700 font-semibold text-xl mt-8 animate-fade-in">
      🎉 Receipt submitted. You did it!
    </div>
  </main>
  <footer class="text-sm text-gray-400 mt-16">
    Designed with love by <a href="https://urbancircles.club" target="_blank" class="text-gray-500 hover:text-gray-700 transition-colors duration-200 underline decoration-gray-300 hover:decoration-gray-500 underline-offset-2">Peter / Urban Circles</a>
  </footer>
  <style>
    @keyframes fade-in {
      from { opacity: 0; transform: translateY(8px); }
      to { opacity: 1; transform: translateY(0); }
    }
    .animate-fade-in {
      animation: fade-in 0.6s ease-out forwards;
    }
  </style>
</body>
</html>
)rawliteral";

// function handleKeyPress(e) {                                   Note: Was located last in <head> section
//   if (e.key === 'Enter' && !e.shiftKey) {
//     e.preventDefault();
//     document.getElementById('receipt-form').dispatchEvent(new Event('submit'));
//   }
// }

// onkeypress="handleKeyPress(event)"                             Note: was located in <textarea> element         

  
  server.send(200, "text/html", html);
}

void handleSubmit() {
  if (server.hasArg("message")) {
    currentReceipt.message = server.arg("message");
    
    // Check if a custom date was provided
    if (server.hasArg("date")) {
      String customDate = server.arg("date");
      currentReceipt.timestamp = formatCustomDate(customDate);
      // Serial.println("Using custom date: " + customDate);
    } else {
      currentReceipt.timestamp = getFormattedDateTime();
      // Serial.println("Using current date");
    }
    
    currentReceipt.hasData = true;
    
    // Serial.println("=== New Receipt Received ===");
    // Serial.println("Message: " + currentReceipt.message);
    // Serial.println("Time: " + currentReceipt.timestamp);
    // Serial.println("============================");
    
    server.send(200, "text/plain", "Receipt received and will be printed!");
  } else {
    server.send(400, "text/plain", "Missing message parameter");
  }
}

void handle404() {
  server.send(404, "text/plain", "Page not found");
}

// === Time Utilities ===
String getFormattedDateTime() {
  timeClient.update();
  
  // Get epoch time
  unsigned long epochTime = timeClient.getEpochTime();
  
  // Convert to struct tm
  time_t rawTime = epochTime;
  struct tm * timeInfo = gmtime(&rawTime);
  
  // Day names and month names
  String dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  String monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  
  // Format: "Sat, 06 Jun 2025"
  String formatted = dayNames[timeInfo->tm_wday] + ", ";
  formatted += String(timeInfo->tm_mday < 10 ? "0" : "") + String(timeInfo->tm_mday) + " ";
  formatted += monthNames[timeInfo->tm_mon] + " ";
  formatted += String(timeInfo->tm_year + 1900);
  
  return formatted;
}

String formatCustomDate(String customDate) {
  // Expected format: YYYY-MM-DD or DD/MM/YYYY or similar
  // This function will try to parse common date formats and return formatted string
  
  String dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  String monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  
  int day = 0, month = 0, year = 0;
  
  // Try to parse YYYY-MM-DD format
  if (customDate.indexOf('-') != -1) {
    int firstDash = customDate.indexOf('-');
    int secondDash = customDate.indexOf('-', firstDash + 1);
    
    if (firstDash != -1 && secondDash != -1) {
      year = customDate.substring(0, firstDash).toInt();
      month = customDate.substring(firstDash + 1, secondDash).toInt();
      day = customDate.substring(secondDash + 1).toInt();
    }
  }
  // Try to parse DD/MM/YYYY format
  else if (customDate.indexOf('/') != -1) {
    int firstSlash = customDate.indexOf('/');
    int secondSlash = customDate.indexOf('/', firstSlash + 1);
    
    if (firstSlash != -1 && secondSlash != -1) {
      day = customDate.substring(0, firstSlash).toInt();
      month = customDate.substring(firstSlash + 1, secondSlash).toInt();
      year = customDate.substring(secondSlash + 1).toInt();
    }
  }
  
  // Validate parsed values
  if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1900 || year > 2100) {
    // Serial.println("Invalid date format, using current date");
    return getFormattedDateTime();
  }
  
  // Calculate day of week (simplified algorithm - may not be 100% accurate for all dates)
  // For a more accurate calculation, you might want to use a proper date library
  int dayOfWeek = 0; // Default to Sunday if we can't calculate
  
  // Format: "Sat, 06 Jun 2025"
  String formatted = dayNames[dayOfWeek] + ", ";
  formatted += String(day < 10 ? "0" : "") + String(day) + " ";
  formatted += monthNames[month - 1] + " ";
  formatted += String(year);
  
  return formatted;
}

// === Printer Functions ===
void initializePrinter() {
  // Initialize printer
  printer.begin(9600);
  delay(500);
  printer.init();
  delay(500);  
  printer.feed(2);
  delay(500);
  
  // Configure printer settings  
  printer.setCodePage(CODEPAGE); // PC437
  delay(500);
  printer.setHeat(HEAT);  // Printer heat/depth
  delay(500);
  printer.setSpeed(SPEED); // Print speed
  delay(500);
  printer.setOrientationUpsideDown(true); // Enable 180° rotation (which also reverses the line order)
  
  printer.println("Printer initialized.");

}

void printReceipt() {
  
  // Print wrapped message first (appears at bottom after rotation)
  printer.printWrappedUpsideDown(currentReceipt.message);
  
  // Print header last (appears at top after rotation)
  printer.printInverted(currentReceipt.timestamp);
  
  // Advance paper
  printer.feed(5);
  
}

void printServerInfo() {
  // Print server info on the thermal printer
  String serverInfo = "Server started at " + WiFi.localIP().toString();
  printer.printWrappedUpsideDown(serverInfo);
  
  printer.printInverted("PRINTER SERVER READY");
  
  printer.feed(5);

}

