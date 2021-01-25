#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h> // read and write from flash memory
#include <WiFi.h>

#include "predict.h"

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 4

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

// enough to test if it fits in flash...

// WiFiClient espClient;
TFT_eSPI tft = TFT_eSPI();

// But move these elsewhere

long pictureNumber = 0;

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  delay(1000);
  //Serial.setDebugOutput(true);
  // Serial.println();

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
  config.fb_count = 1;

  // Init Camera
  Serial.println("init camera?");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  setupModel();

  tft.begin();
  tft.setRotation(3);
  Serial.println("tft begun?");

  //Serial.println("Starting SD Card");
  // if (!SD_MMC.begin("/sdcard", true))
  // {
  //   Serial.println("SD Card Mount Failed");
  //   // no SD Card - go into prediciton & display mode
  //   return;
  // }

  // uint8_t cardType = SD_MMC.cardType();
  // if (cardType == CARD_NONE)
  // {
  //   Serial.println("No SD Card attached");
  //   return;
  // }

  // camera_fb_t *fb = NULL;

  // // Take Picture with Camera
  // fb = esp_camera_fb_get();
  // if (!fb)
  // {
  //   Serial.println("Camera capture failed");
  //   return;
  // }
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.writeLong(0, 0l);
  // pictureNumber = EEPROM.readLong(0) + 1;

  // // Path where new picture will be saved in SD Card
  // String path = "/picture" + String(pictureNumber) + ".jpg";

  // fs::FS &fs = SD_MMC;
  // Serial.printf("Picture file name: %s\n", path.c_str());

  // File file = fs.open(path.c_str(), FILE_WRITE);
  // if (!file)
  // {
  //   Serial.println("Failed to open file in writing mode");
  // }
  // else
  // {
  //   file.write(fb->buf, fb->len); // payload (image), payload length
  //   Serial.printf("Saved file to path: %s\n", path.c_str());
  //   EEPROM.writeLong(0, pictureNumber);
  //   EEPROM.commit();
  // }
  // file.close();
  // esp_camera_fb_return(fb);

  // // Turns off the ESP32-CAM white on-board LED (flash)  connected to GPIO 4
  // pinMode(4, OUTPUT);
  // digitalWrite(4, LOW);
  // rtc_gpio_hold_en(GPIO_NUM_4);

  // delay(2000);
  // Serial.println("Going to sleep now");
  // delay(2000);
  // esp_deep_sleep_start();
  // Serial.println("This will never be printed");
}

void takeAndSavePicture()
{
  camera_fb_t *fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return;
  }
  // initialize EEPROM with predefined size
  pictureNumber = EEPROM.readLong(0) + 1;

  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) + ".jpg";

  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
  }
  else
  {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.writeLong(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);
}

void takeAndPredictPicture()
{
  camera_fb_t *fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return;
  }

  drawer_predictions prediction = runPrediction(fb);

  esp_camera_fb_return(fb);
}

void loop()
{
  // takeAndSavePicture();

  takeAndPredictPicture();

  //Measure time to clear screen
  //drawTime = millis();
  tft.fillScreen(TFT_RED);
  //drawTime = millis() - drawTime;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.drawNumber(drawTime, 10, 100, 4);
  //delay(1000);

  tft.drawNumber(42, 100, 80, 1);

  delay(1000);
}
