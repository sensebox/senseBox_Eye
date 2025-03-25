// bicycle loading animation

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <QRCodeGenerator.h>

#define PIN_QWIIC_SDA 2
#define PIN_QWIIC_SCL 1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int dsplW = 128;
int dsplH = 64;
int prgsW = 120;
int prgsH = 2;
float loadingProgress = 0.0;
QRCode qrcode;

void drawQrCode()
{
  display.clearDisplay();
  display.setTextSize(1);

  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_MEDIUM, "https://www.youtube.com/watch?v=dQw4w9WgXcQ");

  // Text starting point
  int cursor_start_y = 10;
  int cursor_start_x = 4;
  int font_height = 12;

  // QR Code Starting Point
  int offset_x = 62;
  int offset_y = 3;

  for (int y = 0; y < qrcode.size; y++)
  {
    for (int x = 0; x < qrcode.size; x++)
    {
      int newX = offset_x + (x * 2);
      int newY = offset_y + (y * 2);

      if (qrcode_getModule(&qrcode, x, y))
      {
        display.fillRect(newX, newY, 2, 2, 1);
      }
      else
      {
        display.fillRect(newX, newY, 2, 2, 0);
      }
    }
  }
  display.setTextColor(1, 0);
  display.setCursor(cursor_start_x, cursor_start_y + font_height);
  display.println("success!");

  display.display();
}

void drawProgressbar(int x, int y, int width, int height, int progress)
{
  progress = progress > 100 ? 100 : progress; // set the progress value to 100
  progress = progress < 0 ? 0 : progress;     // start the counting to 0-100
  float bar = ((float)(width - 1) / 100) * progress;
  display.drawRect(x, y, width, height + 4, WHITE);
  display.fillRect(x + 2, y + 2, bar - 3, height, WHITE);
}

void setup() {
  Wire.begin(PIN_QWIIC_SDA,PIN_QWIIC_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.setRotation(2);
  display.display();
  delay(100);
  display.clearDisplay();
}

void loop() {
  loadingProgress += 0.1;
  if (loadingProgress > 1.0)
  {
    loadingProgress = 0.0;
    drawQrCode();
    delay(5000000);
  }
  for (int i = 0; i < 36; i++)
  {
    display.clearDisplay();
    display.drawBitmap(32, -10, get_bicycle_loading_bitmap(i), 64, 64, 1); // this displays each frame hex value
    drawProgressbar(4, (dsplH - 12) - prgsH - 8, prgsW, prgsH, loadingProgress * 100);
    display.setCursor(4, dsplH - 12);
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);
    display.println("testing display ...");
    display.display();
    delay(100);
  }
}