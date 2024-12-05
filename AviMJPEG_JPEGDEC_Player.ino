/*
 command: ffmpeg -y -i nino.mp4 -ac 2 -ar 44100 -af loudnorm -c:a mp3 -c:v mjpeg -q:v 7 -vf "fps=30,scale=-1:320:flags=lanczos,crop=170:in_h:(in_w-170)/2:0" nino_30fps_mjpeg.avi
          ffmpeg -y -i nino.mp4 -ac 2 -ar 44100 -af loudnorm -c:a mp3 -c:v mjpeg -q:v 7 -vf "fps=30,scale=320:-1:flags=lanczos,crop=in_w:170:0:(in_h-170)/2" nino_30fps_mjpeg.avi
 */ 

const char *root = "/root";
const char *avi_folder = "/horizon";
// const char *avi_folder = "/vertical";

// Chỉ số file hiện tại (không cần nữa)
// Mảng các tên file AVI
// char *avi_filename[] = {
//     "/root/vid_1_30fps_mjpeg.avi",
//     "/root/dandadan_30fps_mjpeg.avi",
//     "/root/jjk_lifeforce_30fps_mjpeg.avi",
//     "/root/ddd_meme_30fps_mjpeg.avi",
//     "/root/mashle_30fps_mjpeg.avi",
//     "/root/Mclaren765LT_30fps_mjpeg.avi",
//     "/root/csmg_30fps_mjpeg.avi",
//     "/root/shiroko1_30fps_mjpeg.avi",
//     "/root/evangelion_30fps_mjpeg.avi"
// };

// int current_file_index = 0;


// // Kiểm tra giá trị của avi_folder để quyết định cấu hình hiển thị
//   if (strcmp(avi_folder, "/horizon") == 0) // Nếu là chế độ ngang
//   {
//     Arduino_GFX *gfx = new Arduino_ST7789(bus, 42 /* RST */, 1 /* rotation */, true /* IPS */, 170, 320, 35, 0, 35, 0);
//   }
//   else if (strcmp(avi_folder, "/vertical") == 0) // Nếu là chế độ dọc
//   {
//     Arduino_GFX *gfx = new Arduino_ST7789(bus, 42 /* RST */, 0 /* rotation */, true /* IPS */, 170, 320, 35, 0, 35, 0);
//   }
//   else
//   {
//     Serial.println("Invalid avi_folder. Using default configuration.");
//     Arduino_GFX *gfx = new Arduino_ST7789(bus, 42 /* RST */, 1 /* rotation */, true /* IPS */, 170, 320, 35, 0, 35, 0);
//   }

#include "Esp32_s3.h"

#include <FFat.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>

#include "AviFunc_callback.h"

// drawing callback
int drawMCU(JPEGDRAW *pDraw)
{
  unsigned long s = millis();
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  s = millis() - s;
  avi_total_show_video_ms += s;
  avi_total_decode_video_ms -= s;

  return 1;
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("AviMjpeg_JPEGDEC");

  // If display and SD shared same interface, init SPI first
#ifdef SPI_SCK
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
#endif

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  // if (!gfx->begin())
  if (!gfx->begin(GFX_SPEED))
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);
#ifdef CANVAS
  gfx->flush();
#endif

#ifdef GFX_BL
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR < 3)
  ledcSetup(0, 1000, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, 204);
#else  // ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(GFX_BL, 1000, 8, 1);
  ledcWrite(GFX_BL, 204);
#endif // ESP_ARDUINO_VERSION_MAJOR >= 3
#endif // GFX_BL

  // gfx->setTextColor(WHITE, BLACK);
  // gfx->setTextBound(60, 60, 240, 240);

#ifdef AUDIO_MUTE_PIN
  pinMode(AUDIO_MUTE_PIN, OUTPUT);
  digitalWrite(AUDIO_MUTE_PIN, HIGH);
#endif

#if defined(SD_D1)
#define FILESYSTEM SD_MMC
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */, SD_D1, SD_D2, SD_CS /* D3 */);
  if (!SD_MMC.begin(root, false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_SCK)
#define FILESYSTEM SD_MMC
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */);
  if (!SD_MMC.begin(root, true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_CS)
#define FILESYSTEM SD
  if (!SD.begin(SD_CS, SPI, 80000000, "/root"))
#else
#define FILESYSTEM FFat
  if (!FFat.begin(false, root))
  // if (!LittleFS.begin(false, root))
  // if (!SPIFFS.begin(false, root))
#endif
  {
    Serial.println("ERROR: File system mount failed!");
  }
  else
  {
    avi_init();
  }
}

void loop()
{
  File dir = FILESYSTEM.open(avi_folder);
  if (!dir.isDirectory())
  {
    Serial.println("Not a directory");
  }
  else
  {
    // Bước 1: Đọc toàn bộ tệp vào danh sách
    std::vector<std::string> fileList;
    File file = dir.openNextFile();
    while (file)
    {
      if (!file.isDirectory())
      {
        std::string s = file.name();
        if ((s.rfind(".", 0) != 0) && ((int)s.find(".avi", 0) > 0)) // Kiểm tra file .avi
        {
          fileList.push_back(file.name());
        }
      }
      file = dir.openNextFile();
    }
    dir.close();

    // Bước 2: Sắp xếp danh sách theo ký tự đầu tiên
    std::sort(fileList.begin(), fileList.end(), [](const std::string &a, const std::string &b) {
      return a[0] < b[0];
    });

    // Bước 3: Xử lý từng tệp trong danh sách đã sắp xếp
    for (const auto &fileName : fileList)
    {
      std::string filePath = root;
      filePath += avi_folder;
      filePath += "/";
      filePath += fileName;

      if (avi_open((char *)filePath.c_str()))
      {
        Serial.println("AVI start");
        gfx->fillScreen(BLACK);

        avi_start_ms = millis();

        Serial.println("Start play loop");
        while (avi_curr_frame < avi_total_frames)
        {
          if (avi_decode())
          {
            avi_draw(0, 0);
          }
        }

        avi_close();
        Serial.println("AVI end");
      }
    }
  }
}

// void loop()
// {
//   File dir = FILESYSTEM.open(avi_folder);
//   if (!dir.isDirectory())
//   {
//     Serial.println("Not a directory");
//     // delay(5000); // avoid error repeat too fast
//   }
//   else
//   {
//     File file = dir.openNextFile();
//     while (file)
//     {
//       if (!file.isDirectory())
//       {
//         std::string s = file.name();
//         // if ((!s.starts_with(".")) && (s.ends_with(".avi")))
//         if ((s.rfind(".", 0) != 0) && ((int)s.find(".avi", 0) > 0))
//         {
//           s = root;
//           s += file.path();
//           if (avi_open((char *)s.c_str()))
//           {
//             Serial.println("AVI start");
//             gfx->fillScreen(BLACK);

//             avi_start_ms = millis();

//             Serial.println("Start play loop");
//             while (avi_curr_frame < avi_total_frames)
//             {
//               if (avi_decode())
//               {
//                 avi_draw(0, 0);
//                 // avi_draw((gfx->width() - avi_w) / 2, (gfx->height() - avi_h) / 2);
//               }
//             }

//             avi_close();
//             Serial.println("AVI end");

//             // avi_show_stat();
//             // delay(60 * 1000);
//           }
//         }
//       }
//       file = dir.openNextFile();
//     }
//     dir.close();
//   }
// }