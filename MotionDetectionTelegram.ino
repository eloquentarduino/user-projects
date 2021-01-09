// ESP32 CAM
//
// Configurar no Arduino IDE:
//
// Placa:ESP32 Wrover Module
// Upload Speed: 115200
// Flash Frequency: 80MHZ
// Flash Mode: QIO
// Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS) 
//
// Pode ser adapatado para outras placas / cameras
//
// Referencia:
//
// https://eloquentarduino.github.io/2020/12/esp32-cam-motion-detection-with-photo-capture-grayscale-version/
//
// https://github.com/eloquentarduino/EloquentVision/tree/master/examples/EasyMotionDetectionGrayscale


// Definição da Camera

//#define    CAMERA_MODEL_WROVER_KIT
//#define    CAMERA_MODEL_ESP_EYE
//#define    CAMERA_MODEL_M5STACK_PSRAM
//#define    CAMERA_MODEL_M5STACK_WIDE

#define CAMERA_MODEL_AI_THINKER

#define FRAME_SIZE FRAMESIZE_QVGA

#define SOURCE_WIDTH 320

#define SOURCE_HEIGHT 240

#define CHANNELS 1

#define DEST_WIDTH 32

#define DEST_HEIGHT 24

#define BLOCK_VARIATION_THRESHOLD 0.3

#define MOTION_THRESHOLD 0.2



//


#include <FS.h>

#include <SPIFFS.h>

#include "EloquentVision.h"

#include <WiFi.h>

#include <WiFiClientSecure.h>

#include <UniversalTelegramBot.h>

#include <ArduinoJson.h>


// Eloquent Vision - tratamento da imagem

using namespace Eloquent::Vision;

using namespace Eloquent::Vision::IO;

using namespace Eloquent::Vision::ImageProcessing;

using namespace Eloquent::Vision::ImageProcessing::Downscale;

using namespace Eloquent::Vision::ImageProcessing::DownscaleStrategies;


// Eloquent Vision - tratamento da imagem - an easy interface to capture images from the camera

ESP32Camera camera;


// Eloquent Vision - tratamento da imagem - the buffer to store the downscaled version of the image

uint8_t resized[DEST_HEIGHT][DEST_WIDTH];


// Eloquent Vision - tratamento da imagem - the downscaler algorithm
// for more details see https://eloquentarduino.github.io/2020/05/easier-faster-pure-video-esp32-cam-motion-detection

Cross<SOURCE_WIDTH, SOURCE_HEIGHT, DEST_WIDTH, DEST_HEIGHT> crossStrategy;


// Eloquent Vision - tratamento da imagem - the downscaler container

Downscaler<SOURCE_WIDTH, SOURCE_HEIGHT, CHANNELS, DEST_WIDTH, DEST_HEIGHT> downscaler(&crossStrategy);


// Eloquent Vision - tratamento da imagem - the motion detection algorithm

MotionDetection<DEST_WIDTH, DEST_HEIGHT> motion;

JpegWriter<SOURCE_WIDTH, SOURCE_HEIGHT> jpegWriter;

bool debounceMotion(bool touch = false);

void printFilesize(const char *filename);


/**
 *
 */



// >>>  Telegram

// Para criar Bot
// Procure por BotFather
// mande: /start
// Será solicitado nome do bot, que como irá aparecer nas mensagens
// Depois será solicitado o user name do bot. IMPORTANTE: tem que termiinar com Bot
// No texto de pronto (Done) tem um link curto (ex.: t.me/nomedobot) para começar chat com o bot
// Também é informado o token do bot

#define BOT_TOKEN "XXXXXXXXXXXX"


// Para identificar ID no Telegrama
// Mande msg para @myidbot 
// para iniciar = /start 
// ID pessoal = /getid
// ID grupo = /getgroupid
// para usar grupo tem que add o BOT no Grupo

#define chat_id "xxxxxxx"


// >>>  Wifi

#define WIFI_SSID "xxxxx"
#define WIFI_PASSWORD "XXXXXXXXXXXX"

WiFiClientSecure secured_client;

UniversalTelegramBot bot(BOT_TOKEN, secured_client);


/** @fix define global file variable and required functions for Telegram **/
File imageFile;


bool isMoreDataAvailable() {
  return imageFile.available();
}

byte getNextByte() {
  return imageFile.read();
}



//  >>> Setup

 
void setup() {
    
    Serial.begin(115200);


 // attempt to connect to Wifi network:

  Serial.print("Connectando...");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connectado. IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Atualizando Relógio: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  bot.sendMessage(chat_id, "ESP32CAM-RES iniciado", "");

// Camera
 
    SPIFFS.begin(true);

    camera.begin(FRAME_SIZE, PIXFORMAT_GRAYSCALE);
    
    motion.setBlockVariationThreshold(BLOCK_VARIATION_THRESHOLD);


}

/**
 *
 */

void loop() {

    uint32_t start = millis();
    
    camera_fb_t *frame = camera.capture();

    // resize image and detect motion
    
    downscaler.downscale(frame->buf, resized);

    motion.update(resized);
    
    motion.detect();

    // compute FPS
    
//    Serial.print(1000.0f / (millis() - start));
//    
//    Serial.println(" fps");

    // on motion detected, save image to filesystem
    
    if (motion.ratio() > MOTION_THRESHOLD) {
    
        Serial.println("Motion detected");

        // save image
        
        if (debounceMotion()) {
            
            // take a new pic in the hope it is less affected by the motion noise
            // (you may comment this out if you want)
        
            delay(500);
    
            frame = camera.capture();

            // write as jpeg
            
            File imageFile = SPIFFS.open("/capture.jpg", "wb");
            
            // you can tweak this value as per your needs
            
            uint8_t quality = 10;

            Serial.println("The image will be saved as /capture.jpg");
            
            jpegWriter.writeGrayscale(imageFile, frame->buf, quality);
            
            imageFile.close();
            
            printFilesize("/capture.jpg");

            delay(500);

//          handleNewMessages();       // envia foto


            Serial.println("chama readFile");

            readFile(SPIFFS, "/capture.jpg"); // envia foto




            debounceMotion(true);
        }
       
    }
}

/**
 * Debounce repeated motion detections
 * @return
 */
 
bool debounceMotion(bool touch) {
 
    static uint32_t lastMotion = 0;

    // update last tick
    
    if (lastMotion == 0 || touch)
        lastMotion = millis();

    // debounce
    
    if (millis() - lastMotion > 5000) {
        lastMotion = millis();

        return true;
    }

    return false;
}


/**
 * Print file size (for debug)
 * @param filename
 */
 
void printFilesize(const char *filename) {
    File file = SPIFFS.open(filename, "r");

    Serial.print(filename);
    
    Serial.print(" size is ");
    
    Serial.print(file.size() / 1000);
    
    Serial.println(" kb");

    file.close();
}


//


void readFile(fs::FS &fs, const char * path){

    Serial.println("inicio readFile");


    Serial.printf("Reading file: %s\r\n", path);

    /** @fix use global variable **/
    imageFile = SPIFFS.open("/capture.jpg", "rb");


    if(!imageFile || imageFile.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
   
      String  sent = bot.sendPhotoByBinary(chat_id, "image/jpeg", imageFile.size(),
                                           isMoreDataAvailable,
                                           getNextByte, nullptr, nullptr);


            
        if (sent) {
          Serial.println("was successfully sent");
        } else {
          Serial.println("was not sent");
        }
        
imageFile.close();
}
