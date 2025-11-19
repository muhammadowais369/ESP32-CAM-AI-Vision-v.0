/*
  ESP32-CAM Image Analysis with OpenRouter API (Vision Models)

  This code captures an image using the ESP32-CAM module, processes it, 
  and sends it to OpenRouter's vision models for analysis.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Base64.h>
#include "esp_camera.h"
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

// ========== PUT YOUR OPENROUTER API KEY HERE ==========
const String openRouterApiKey = "API KEY";
// Get free API key from: https://openrouter.ai/keys
// ======================================================

// Pin definitions for ESP32-CAM AI-Thinker module
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define BUTTON_PIN 13
#define BUZZER_PIN 2  // Buzzer connected to GPIO2

// Function to encode image to Base64
String encodeImageToBase64(const uint8_t* imageData, size_t imageSize) {
  return base64::encode(imageData, imageSize);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("\n\n=== ESP32-CAM-AI-Vision-v.0 ===");
  Serial.println("MUHMMAD OWAIS 369");
  Serial.println("Using OpenRouter Vision API");
  Serial.println("=============================\n");
  delay(3000);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi Connected!");
  delay(2000);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  Serial.println("Camera Initialized");
  delay(2000);

  Serial.println("Press button to capture");
}

void captureAndAnalyzeImage() {
  Serial.println("Capturing image...");

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  esp_camera_fb_return(fb);
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("Image captured");
  String base64Image = encodeImageToBase64(fb->buf, fb->len);

  beep();
  esp_camera_fb_return(fb);

  if (base64Image.isEmpty()) {
    Serial.println("Failed to encode the image!");
    return;
  }
  
  AnalyzeImageWithOpenRouter(base64Image);
}

void AnalyzeImageWithOpenRouter(const String& base64Image) {
  Serial.println("Sending image to OpenRouter...");
  Serial.println("Processing...");

  String result;

  // Try different vision models - start with the first one
  String models[] = { 
    "openai/gpt-4o-mini"   // OpenAI vision model (may have costs)
     };

  bool success = false;
  
  for (int i = 0; i < 4 && !success; i++) {
    Serial.println("Trying model: " + models[i]);
    success = sendOpenRouterRequest(models[i], base64Image, result);
    
    if (!success) {
      Serial.println("Model " + models[i] + " failed, trying next...");
      delay(1000);
    }
  }

  if (success) {
    Serial.println("\n=== IMAGE ANALYSIS RESULT ===");
    Serial.println(result);
    Serial.println("==============================\n");
    Serial.println("Press button to capture again");
  } else {
    Serial.println("All models failed. Last error: " + result);
  }
}

bool sendOpenRouterRequest(const String& model, const String& base64Image, String& result) {
  HTTPClient http;
  
  String url = "https://openrouter.ai/api/v1/chat/completions";
  http.begin(url);
  
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + openRouterApiKey);
  http.addHeader("HTTP-Referer", "https://github.com/muhammadowais369");
  http.addHeader("X-Title", "ESP32-CAM-AI-Vision-v.0");
  
  http.setTimeout(30000);

  DynamicJsonDocument doc(16384);
  doc["model"] = model;
  
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject message = messages.createNestedObject();
  message["role"] = "user";
  
  JsonArray content = message.createNestedArray("content");
  
  JsonObject textContent = content.createNestedObject();
  textContent["type"] = "text";
  textContent["text"] = "Analyze and describe this image in detail. What can you see in this image?";
  
  JsonObject imageContent = content.createNestedObject();
  imageContent["type"] = "image_url";
  JsonObject imageUrl = imageContent.createNestedObject("image_url");
  imageUrl["url"] = "data:image/jpeg;base64," + base64Image;

  doc["max_tokens"] = 500;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.print("Payload size: ");
  Serial.println(jsonPayload.length());

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode == 200) {
    String response = http.getString();
    http.end();
    
    DynamicJsonDocument responseDoc(8192);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error && responseDoc.containsKey("choices") && 
        responseDoc["choices"].size() > 0 &&
        responseDoc["choices"][0].containsKey("message") &&
        responseDoc["choices"][0]["message"].containsKey("content")) {
      
      result = responseDoc["choices"][0]["message"]["content"].as<String>();
      return true;
    } else {
      result = "Failed to parse response";
      return false;
    }
  } else {
    result = http.getString();
    http.end();
    return false;
  }
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed! Capturing image...");
    captureAndAnalyzeImage();
    delay(1000);
  }
}

void beep(){
  digitalWrite(2,HIGH);
  delay(300);
  digitalWrite(2,LOW);
}