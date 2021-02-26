#define DEBUG_MODE false

#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"

#include "predict.h"
#include "ui.h"
#include "part_database.h"

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

// Pin definition for CAMERA_MODEL_AI_THINKER
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

#define BACKLIGHT_PIN 1
#define INACTION_TIMEOUT_FRAMES 10

// enough to test if it fits in flash...

// WiFiClient espClient;
TFT_eSPI tft = TFT_eSPI();

// static uint16_t tftBuffer[160 * 60] = {0};

uint8_t *bmp_buf = nullptr;
size_t bmp_buf_len = 0;

long pictureNumber = 0;
bool hasSDCard = false;
int noInteractionFrames = 0;

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  if (DEBUG_MODE)
  {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting SD Card");
  }

  if (SD_MMC.begin("/sdcard", true))
  {
    if (DEBUG_MODE)
      Serial.println("SD Card Mounted");
    // no SD Card - go into prediciton & display mode
    hasSDCard = true;
  }
  else
  {
    if (DEBUG_MODE)
      Serial.println("No SD Card detected");
    hasSDCard = false;
    delay(1000);
  }

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
  config.pixel_format = PIXFORMAT_RGB888;

  config.frame_size = FRAMESIZE_QQVGA;
  config.fb_count = hasSDCard ? 2 : 1;

  // Init Camera
  if (DEBUG_MODE)
    Serial.println("init camera?");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    if (DEBUG_MODE)
      Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  if (!hasSDCard)
  {
    setupModel();
    tft.begin();
    tft.setRotation(3);
    tft.setSwapBytes(true);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    setupEEPROMAndWiFi();
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);
  }
}

void takeAndSavePicture()
{
  camera_fb_t *fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb)
  {
    if (DEBUG_MODE)
      Serial.println("Camera capture failed");
    return;
  }
  pictureNumber++;

  bool converted = frame2bmp(fb, &bmp_buf, &bmp_buf_len);
  esp_camera_fb_return(fb);

  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) + ".bmp";

  fs::FS &fs = SD_MMC;
  if (DEBUG_MODE)
    Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file)
  {
    if (DEBUG_MODE)
      Serial.println("Failed to open file in writing mode");
  }
  else
  {
    file.write(bmp_buf, bmp_buf_len); // payload (image), payload length
    if (DEBUG_MODE)
      Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();
  free(bmp_buf);
}

float *takeAndPredictPicture()
{
  camera_fb_t *fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb)
  {
    if (DEBUG_MODE)
      Serial.println("Camera capture failed");
    return {0};
  }

  float *results = runPrediction(fb->buf, fb->len);
  if (DEBUG_MODE)
    Serial.println("I've got back a prediction");

  //writePredictionFrameTo16BitBuffer(fb->buf, fb->len, tftBuffer);

  //Serial.println("has written the to the tftBuffer");
  //tft.pushImage(120 - 80, 0, 160, 60, tftBuffer);

  esp_camera_fb_return(fb);
  return results;
}

void loop()
{
  if (hasSDCard)
  {
    takeAndSavePicture();
    delay(500);
    return;
  }
  runOTALoop();

  float *results = takeAndPredictPicture();

  int most_likely_drawer = -1;
  float max_value_so_far = 0.01f;

  for (int i = 0; i < 40; i++)
  {
    if (results[i] > max_value_so_far)
    {
      max_value_so_far = results[i];
      most_likely_drawer = i;
    }
  }

  if (most_likely_drawer < 1)
  {
    if (noInteractionFrames < INACTION_TIMEOUT_FRAMES)
    {
      noInteractionFrames++;
    }
  }
  else
  {
    noInteractionFrames = 0;
  }

  digitalWrite(BACKLIGHT_PIN, noInteractionFrames != INACTION_TIMEOUT_FRAMES ? HIGH : LOW);
  int results_x = 120 - (getResultsTotalWidth() / 2);
  int results_y = 240 - getResultsTotalHeight();

  tft.fillScreen(TFT_BLACK);

  drawDrawersForResults(results_x, results_y, results, tft);

  tft.drawString(getWiFiStatus(), 0, 0);
  // tft.drawNumber(most_likely_drawer, 100, 80, 4);
  if (most_likely_drawer > 0)
  {
    String text = getPartText(most_likely_drawer);
    int newLineIndex = text.indexOf("\n");
    if (newLineIndex == -1)
    {
      tft.drawString(text, 10, 50, 4);
    }
    else
    {
      tft.drawString(text.substring(0, newLineIndex), 10, 50, 4);
      tft.drawString(text.substring(newLineIndex), 10, 76, 4);
    }
  }
  delay((most_likely_drawer > 0) ? 0 : 1000);
}
