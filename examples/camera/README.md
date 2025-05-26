### Pinout
```cpp
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5

#define Y9_GPIO_NUM    16 // D7
#define Y8_GPIO_NUM    17 // D6
#define Y7_GPIO_NUM    18 // D5
#define Y6_GPIO_NUM    12 // D4
#define Y5_GPIO_NUM    10 // D3
#define Y4_GPIO_NUM    8  // D2
#define Y3_GPIO_NUM    9  // D1
#define Y2_GPIO_NUM    11 // D0
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13
```

# Tested Camera models
The senseBox Eye comes with an OV2640. But its possible to attach other cameras that use the DVP too.

| model   | max resolution | color type | output format                                                | Len Size |
| ------- | -------------- | ---------- | ------------------------------------------------------------ | -------- |
| OV2640  | 1600 x 1200    | color      | YUV(422/420)/YCbCr422<br>RGB565/555<br>8-bit compressed data<br>8/10-bit Raw RGB data | 1/4"     |
| OV7725  | 640 x 480      | color      | Raw RGB<br/>GRB 422<br/>RGB565/555/444<br/>YCbCr 422         | 1/4"     |
| OV5640  | 2592 x 1944    | color      | RAW RGB<br/>RGB565/555/444<br/>CCIR656<br/>YUV422/420<br/>YCbCr422<br/>compression | 1/4"     |

Depending on how much ram is already taken up by the rest of the script, the resolution of the camera might have to be reduced to prevent an overflow (additionally it also helps to set the fb_count of the config to 1 instead of higher numbers). So the max resolution can often not be achieved.

# Examples
### CameraWebServer
Default example from the esp32 BSP. Streams camera images over Wifi.

Requires version 3.0.7 of the BSP for face detection and recognition (newer versions dont include the model zoo anymore). Dont forget to adjust the Wifi credentials.

### CameraWebServer
Same as CameraWebServer but with custom edge impulse model instead of face detection. Streaming is broken, only capturing single frames possible.

### camera-display
Continuously print current camera image on the display.

### camera-sd
Capture a single camera image (as JPEG) and save it to sd-card. Requires PSRAM turned on.

### ov7725
Continuously print current camera image on the display. If the "SW"-Button is pressed, capture a single camera image (as RGB565, because ov7725 doesnt natively support JPEG) and save it to sd-card. Requires PSRAM turned on.

### ov5640
Continuously print current camera image on the display. If the "SW"-Button is pressed, capture a single camera image (as RGB565, because ov7725 doesnt natively support JPEG) and save it to sd-card. The highest resolution achieved with this script is 2560x1600px (4.1MP). Requires PSRAM turned on.

# esp32-camera
The underlying camera driver used in these examples is [esp32-camera](https://github.com/espressif/esp32-camera). 

It supports the following resolution shorthands:
| key                | resolution    |
|--------------------|---------------|
| FRAMESIZE_96X96    | 96x96         |
| FRAMESIZE_QQVGA    | 160x120       |
| FRAMESIZE_128X128  | 128x128       |
| FRAMESIZE_QCIF     | 176x144       |
| FRAMESIZE_HQVGA    | 240x176       |
| FRAMESIZE_240X240  | 240x240       |
| FRAMESIZE_QVGA     | 320x240       |
| FRAMESIZE_320X320  | 320x320       |
| FRAMESIZE_CIF      | 400x296       |
| FRAMESIZE_HVGA     | 480x320       |
| FRAMESIZE_VGA      | 640x480       |
| FRAMESIZE_SVGA     | 800x600       |
| FRAMESIZE_XGA      | 1024x768      |
| FRAMESIZE_HD       | 1280x720      |
| FRAMESIZE_SXGA     | 1280x1024     |
| FRAMESIZE_UXGA     | 1600x1200     |
| **--- 3MP Sensors ---** |    
| FRAMESIZE_FHD      | 1920x1080     |
| FRAMESIZE_P_HD     | 720x1280      |
| FRAMESIZE_P_3MP    | 864x1536      |
| FRAMESIZE_QXGA     | 2048x1536     |
| **--- 5MP Sensors ---** |
| FRAMESIZE_QHD      | 2560x1440     |
| FRAMESIZE_WQXGA    | 2560x1600     |
| FRAMESIZE_P_FHD    | 1080x1920     |
| FRAMESIZE_QSXGA    | 2560x1920     |
| FRAMESIZE_5MP      | 2592x1944     |

The following formats are supported:
| key                  | pixel format           |
|----------------------|------------------------|
| PIXFORMAT_RGB565     | 2BPP / RGB565          |
| PIXFORMAT_YUV422     | 2BPP / YUV422          |
| PIXFORMAT_YUV420     | 1.5BPP / YUV420        |
| PIXFORMAT_GRAYSCALE  | 1BPP / Grayscale       |
| PIXFORMAT_JPEG       | JPEG / Compressed      |
| PIXFORMAT_RGB888     | 3BPP / RGB888          |
| PIXFORMAT_RAW        | RAW                    |
| PIXFORMAT_RGB444     | 3BP2P / RGB444         |
| PIXFORMAT_RGB555     | 3BP2P / RGB555         |

The driver also supports a multitude of camera settings for image capture ([tutorial](https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/)).