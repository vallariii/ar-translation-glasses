#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// =========================================
// 1. CONFIGURATION
// =========================================
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
String server_url    = "http://YOUR_PC_IP:5000/upload"; // <--- PUT YOUR PC IP HERE

// =========================================
// 2. HARDWARE PINS
// =========================================
// INMP441 Microphone
#define I2S_WS 15
#define I2S_SD 16
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0

// OLED Display
#define OLED_SDA 8
#define OLED_SCL 9

// Button
#define BUTTON_PIN 2

// =========================================
// 3. MEMORY SETTINGS (Optimized for Internal RAM)
// =========================================
#define SAMPLE_RATE 16000

// CHANGED: Reduced to 3 seconds to fit in internal memory
#define DURATION_SEC 3

#define HEADER_SIZE 44
#define AUDIO_DATA_SIZE (SAMPLE_RATE * DURATION_SEC * 2)
#define TOTAL_BUFFER_SIZE (HEADER_SIZE + AUDIO_DATA_SIZE)

uint8_t *bigBuffer; // Holds Header + Audio
size_t bytesRecorded = 0;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- OLED Setup ---
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED Failed"));
    for(;;);
  }
  
  showStatus("WiFi...", "Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  showStatus("Ready", "Hold Btn (3s)");

  // --- Memory Allocation ---
  // We use standard malloc() because you do not have PSRAM
  bigBuffer = (uint8_t *)malloc(TOTAL_BUFFER_SIZE);
  
  if(bigBuffer == NULL) {
    showStatus("CRITICAL ERROR", "RAM FULL!");
    Serial.println("Not enough internal RAM!");
    while(1);
  } else {
    Serial.println("RAM Allocated Successfully");
  }

  setupI2S();
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    recordAudio();
    sendAudio();
    // Debounce: Wait for release
    while(digitalRead(BUTTON_PIN) == LOW) { delay(10); } 
  }
}

// --- I2S SETUP ---
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, 
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8, 
    .dma_buf_len = 64,
    .use_apll = false, 
    .tx_desc_auto_clear = false, 
    .fixed_mclk = 0
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK, 
    .ws_io_num = I2S_WS, 
    .data_out_num = -1, 
    .data_in_num = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

// --- RECORDING FUNCTION ---
void recordAudio() {
  showStatus("Recording...", "Speak Now");
  
  // Skip the first 44 bytes (Reserved for Header)
  int16_t *samples = (int16_t *)(bigBuffer + HEADER_SIZE);
  
  size_t samplesRead = 0;
  size_t maxSamples = AUDIO_DATA_SIZE / 2;
  int32_t rawSample = 0; 
  size_t bytesIn = 0;

  // Clear buffer before recording to avoid noise
  i2s_zero_dma_buffer(I2S_PORT);

  while (digitalRead(BUTTON_PIN) == LOW && samplesRead < maxSamples) {
    esp_err_t result = i2s_read(I2S_PORT, &rawSample, sizeof(rawSample), &bytesIn, portMAX_DELAY);
    if (result == ESP_OK && bytesIn > 0) {
      // Shift 32-bit data to 16-bit
      samples[samplesRead] = (int16_t)(rawSample >> 14); 
      samplesRead++;
    }
  }
  bytesRecorded = samplesRead * 2;
  Serial.print("Recorded bytes: ");
  Serial.println(bytesRecorded);
}

// --- SENDING FUNCTION ---
void sendAudio() {
  showStatus("Sending...", "Please Wait");
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // CHANGED: Increased timeout to 20 seconds to fix "Bad Request" / Dropouts
    http.setTimeout(20000); 
    
    http.begin(server_url);
    http.addHeader("Content-Type", "audio/wav");

    // --- CONSTRUCT WAV HEADER ---
    uint32_t fileSize = bytesRecorded + 36;
    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * 2;
    
    uint8_t header[44] = {
      'R', 'I', 'F', 'F', 
      (uint8_t)(fileSize & 0xFF), (uint8_t)((fileSize >> 8) & 0xFF), (uint8_t)((fileSize >> 16) & 0xFF), (uint8_t)((fileSize >> 24) & 0xFF),
      'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 
      16, 0, 0, 0, 1, 0, 1, 0, 
      (uint8_t)(sampleRate & 0xFF), (uint8_t)((sampleRate >> 8) & 0xFF), (uint8_t)((sampleRate >> 16) & 0xFF), (uint8_t)((sampleRate >> 24) & 0xFF),
      (uint8_t)(byteRate & 0xFF), (uint8_t)((byteRate >> 8) & 0xFF), (uint8_t)((byteRate >> 16) & 0xFF), (uint8_t)((byteRate >> 24) & 0xFF),
      2, 0, 16, 0, 
      'd', 'a', 't', 'a', 
      (uint8_t)(bytesRecorded & 0xFF), (uint8_t)((bytesRecorded >> 8) & 0xFF), (uint8_t)((bytesRecorded >> 16) & 0xFF), (uint8_t)((bytesRecorded >> 24) & 0xFF)
    };

    // Copy Header into the reserved front space
    memcpy(bigBuffer, header, 44);

    // --- SEND POST REQUEST ---
    int httpResponseCode = http.POST(bigBuffer, HEADER_SIZE + bytesRecorded);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response); // Debug print
      
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        const char* final_text = doc["text"];
        showStatus("Translation:", final_text);
      } else {
        showStatus("Error", "JSON Fail");
      }
    } else {
      Serial.print("Error Code: ");
      Serial.println(httpResponseCode);
      showStatus("Connection", "Failed");
    }
    http.end();
  } else {
    showStatus("Error", "No WiFi");
  }
}

void showStatus(const char* title, const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0); display.println(title);
  display.setCursor(0, 16); display.println(msg);
  display.display();
}
