// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "esp32-hal-ledc.h"
#include "sdkconfig.h"
#include "camera_index.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#endif

// Car Detection will not work on boards without (or with disabled) PSRAM
#ifdef BOARD_HAS_PSRAM
// Car Recognition takes upward from 15 seconds per frame on chips other than ESP32S3
// Makes no sense to have it enabled for them
#if CONFIG_IDF_TARGET_ESP32S3
#define CONFIG_ESP_CAR_DETECT_ENABLED      1
#else
#define CONFIG_ESP_CAR_DETECT_ENABLED      1
#endif
#else
#define CONFIG_ESP_CAR_DETECT_ENABLED      1
#endif

#if CONFIG_ESP_CAR_DETECT_ENABLED

#include <car-detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

#endif


typedef struct {
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

#if CONFIG_ESP_CAR_DETECT_ENABLED

static int8_t detection_enabled = 0;
/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture

#endif

typedef struct {
  size_t size;   //number of values used for filtering
  size_t index;  //current value index
  size_t count;  //value count
  int sum;
  int *values;  //array to be filled with values
} ra_filter_t;

static ra_filter_t ra_filter;

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size) {
  memset(filter, 0, sizeof(ra_filter_t));

  filter->values = (int *)malloc(sample_size * sizeof(int));
  if (!filter->values) {
    return NULL;
  }
  memset(filter->values, 0, sample_size * sizeof(int));

  filter->size = sample_size;
  return filter;
}

// ARDUHAL_LOG_LEVEL_INFO
static int ra_filter_run(ra_filter_t *filter, int value) {
  if (!filter->values) {
    return value;
  }
  filter->sum -= filter->values[filter->index];
  filter->values[filter->index] = value;
  filter->sum += filter->values[filter->index];
  filter->index++;
  filter->index = filter->index % filter->size;
  if (filter->count < filter->size) {
    filter->count++;
  }
  return filter->sum / filter->count;
}

#if CONFIG_ESP_CAR_DETECT_ENABLED

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
  // we already have a RGB888 buffer, so recalculate offset into pixel index
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;

  while (pixels_left != 0) {
    // Swap BGR to RGB here
    // due to https://github.com/espressif/esp32-camera/issues/379
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];

    // go to the next pixel
    out_ptr_ix++;
    pixel_ix+=3;
    pixels_left--;
  }
  // and done!
  return 0;
}


static void draw_car_boxes(fb_data_t *fb, ei_impulse_result_t *result) {
  int x, y, w, h;
  uint32_t color = 0x0000FF00;
  if (fb->bytes_per_pixel == 2) {
    //color = ((color >> 8) & 0xF800) | ((color >> 3) & 0x07E0) | (color & 0x001F);
    color = ((color >> 16) & 0x001F) | ((color >> 3) & 0x07E0) | ((color << 8) & 0xF800);
  }
  int i = 0;
  for (uint32_t i = 0; i < result->bounding_boxes_count; i++) {
    ei_impulse_result_bounding_box_t bb = result->bounding_boxes[i];
    ei_printf("Found label %s with confidence %f at %d,%d %dx%d\n",
              result->bounding_boxes[i].label,
              result->bounding_boxes[i].value,
              bb.x, bb.y, bb.width, bb.height);
    // rectangle box
    x = (int)bb.x;
    y = (int)bb.y;
    w = (int)bb.width;
    h = (int)bb.height;
    if ((x + w) > fb->width) {
      w = fb->width - x;
    }
    if ((y + h) > fb->height) {
      h = fb->height - y;
    }
    fb_gfx_drawFastHLine(fb, x, y, w, color);
    fb_gfx_drawFastHLine(fb, x, y + h - 1, w, color);
    fb_gfx_drawFastVLine(fb, x, y, h, color);
    fb_gfx_drawFastVLine(fb, x + w - 1, y, h, color);

    // // Draw label and confidence
    // char label_buf[64];
    // snprintf(label_buf, sizeof(label_buf), "%s: %.2f", bb.label, bb.value);
    // // Draw text at the top-left corner of the bounding box
    // fb_gfx_print(fb, x, y - 12 > 0 ? y - 12 : y, color, label_buf);
  }
}

#endif


static esp_err_t bmp_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  uint64_t fr_start = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
  fb = esp_camera_fb_get();
  if (!fb) {
    log_e("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/x-windows-bmp");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.bmp");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char ts[32];
  snprintf(ts, 32, "%lld.%06ld", fb->timestamp.tv_sec, fb->timestamp.tv_usec);
  httpd_resp_set_hdr(req, "X-Timestamp", (const char *)ts);

  uint8_t *buf = NULL;
  size_t buf_len = 0;
  bool converted = frame2bmp(fb, &buf, &buf_len);
  esp_camera_fb_return(fb);
  if (!converted) {
    log_e("BMP Conversion failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  res = httpd_resp_send(req, (const char *)buf, buf_len);
  free(buf);
  uint64_t fr_end = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
  log_i("BMP: %llums, %uB", (uint64_t)((fr_end - fr_start) / 1000), buf_len);
  return res;
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (!index) {
    j->len = 0;
  }
  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
    return 0;
  }
  j->len += len;
  return len;
}

static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  int64_t fr_start = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO

  fb = esp_camera_fb_get();

  if (!fb) {
    log_e("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char ts[32];
  snprintf(ts, 32, "%lld.%06ld", fb->timestamp.tv_sec, fb->timestamp.tv_usec);
  httpd_resp_set_hdr(req, "X-Timestamp", (const char *)ts);

#if CONFIG_ESP_CAR_DETECT_ENABLED
  size_t out_len, out_width, out_height; // TODO: needed?
  bool s; // TODO: needed?
  snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
  // check if allocation was successful
  if(snapshot_buf == nullptr) {
    ei_printf("ERR: Failed to allocate snapshot buffer!\n");
    delay(5000);
  }
  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;
  bool detected = false; // ARDUHAL_LOG_LEVEL_INFO
  if (!detection_enabled || fb->width > 400) {
#endif
    size_t fb_len = 0; // ARDUHAL_LOG_LEVEL_INFO
    if (fb->format == PIXFORMAT_JPEG) {
      fb_len = fb->len; // ARDUHAL_LOG_LEVEL_INFO
      res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    } else {
      jpg_chunking_t jchunk = {req, 0};
      res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
      httpd_resp_send_chunk(req, NULL, 0);
      fb_len = jchunk.len;  // ARDUHAL_LOG_LEVEL_INFO
    }
    esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();  // ARDUHAL_LOG_LEVEL_INFO
    log_i("JPG: %uB %ums", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
    return res;
#if CONFIG_ESP_CAR_DETECT_ENABLED
  }

  jpg_chunking_t jchunk = {req, 0};

  if (fb->format == PIXFORMAT_RGB565) {
    snapshot_buf = fb->buf; // TODO: dont use snapshot_buf here?
    
    bool do_resize = false;
    if ((fb->width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
      || (fb->height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
      do_resize = true;
    }
    if (do_resize) {
      ei::image::processing::crop_and_interpolate_rgb888(
      snapshot_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      snapshot_buf,
      fb->width,
      fb->height);
    }

    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        delay(5000);
    }

    if (result.bounding_boxes_count > 0) {
      fb_data_t rfb;
      rfb.width = fb->width;
      rfb.height = fb->height;
      rfb.data = fb->buf;
      rfb.bytes_per_pixel = 2;
      rfb.format = FB_RGB565;
      detected = true; // ARDUHAL_LOG_LEVEL_INFO
      draw_car_boxes(&rfb, &result);
    }
    s = fmt2jpg_cb(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 90, jpg_encode_stream, &jchunk);
    esp_camera_fb_return(fb);
  } else {
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;
    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);
    if(!converted){
      ei_printf("Conversion failed\n");
      return false;
    }
    bool do_resize = false;
    if ((out_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
      || (out_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
      do_resize = true;
    }

    if (do_resize) {
      ei::image::processing::crop_and_interpolate_rgb888(
      snapshot_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      snapshot_buf,
      out_width,
      out_height);
    }

    fb_data_t rfb;
    rfb.width = out_width;
    rfb.height = out_height;
    rfb.data = snapshot_buf;
    rfb.bytes_per_pixel = 3;
    rfb.format = FB_BGR888;

    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        delay(5000);
    }

    if (result.bounding_boxes_count > 0) {
      detected = true; // ARDUHAL_LOG_LEVEL_INFO
      draw_car_boxes(&rfb, &result);
    }

    s = fmt2jpg_cb(snapshot_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    free(snapshot_buf);
  }

  if (!s) {
    log_e("JPEG compression failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  int64_t fr_end = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
  return res;
#endif
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  struct timeval _timestamp;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[128];
#if CONFIG_ESP_CAR_DETECT_ENABLED
  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;
  bool detected = false; // ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_ready = 0; // ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_recognize = 0; // ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_encode = 0; // ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_car = 0; // ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_start = 0; // ARDUHAL_LOG_LEVEL_INFO
  size_t out_len = 0, out_width = 0, out_height = 0;
  bool s = false;
#endif

  static int64_t last_frame = 0;
  if (!last_frame) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "X-Framerate", "60");

  while (true) {
  ei_printf("free heap: %d\n", esp_get_free_heap_size());
#if CONFIG_ESP_CAR_DETECT_ENABLED
    detected = false; // ARDUHAL_LOG_LEVEL_INFO
#endif

    fb = esp_camera_fb_get();
    if (!fb) {
      log_e("Camera capture failed");
      res = ESP_FAIL;
    } else {
      _timestamp.tv_sec = fb->timestamp.tv_sec;
      _timestamp.tv_usec = fb->timestamp.tv_usec;
#if CONFIG_ESP_CAR_DETECT_ENABLED
      fr_start = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
      fr_ready = fr_start; // ARDUHAL_LOG_LEVEL_INFO
      fr_encode = fr_start; // ARDUHAL_LOG_LEVEL_INFO
      fr_recognize = fr_start; // ARDUHAL_LOG_LEVEL_INFO
      fr_car = fr_start; // ARDUHAL_LOG_LEVEL_INFO
      if (!detection_enabled || fb->width > 400) {
#endif
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            log_e("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
#if CONFIG_ESP_CAR_DETECT_ENABLED
      } else {
        if (fb->format == PIXFORMAT_RGB565
        ) {
          fr_ready = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
          bool do_resize = false;
          if ((fb->width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
            || (fb->height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
            do_resize = true;
          }

          snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
          // check if allocation was successful
          if(snapshot_buf == nullptr) {
            ei_printf("ERR: Failed to allocate snapshot buffer!\n");
            delay(5000);
            return res;
          }
          snapshot_buf = fb->buf; // TODO: dont use snapshot_buf here?
          if (do_resize) {
            ei::image::processing::crop_and_interpolate_rgb888(
            snapshot_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            snapshot_buf,
            fb->width,
            fb->height);
          }

          ei_impulse_result_t result = { 0 };

          EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
          if (err != EI_IMPULSE_OK) {
              ei_printf("ERR: Failed to run classifier (%d)\n", err);
              delay(5000);
          }
#if CONFIG_ESP_CAR_DETECT_ENABLED
          fr_car = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
          fr_recognize = fr_car; // ARDUHAL_LOG_LEVEL_INFO
#endif
          if (result.bounding_boxes_count > 0) {
            fb_data_t rfb;
            rfb.width = fb->width;
            rfb.height = fb->height;
            rfb.data = fb->buf;
            rfb.bytes_per_pixel = 2;
            rfb.format = FB_RGB565;
            detected = true; // ARDUHAL_LOG_LEVEL_INFO
            draw_car_boxes(&rfb, &result);
          }
          s = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!s) {
            log_e("fmt2jpg failed");
            res = ESP_FAIL;
          }
#if CONFIG_ESP_CAR_DETECT_ENABLED
          fr_encode = esp_timer_get_time();  // ARDUHAL_LOG_LEVEL_INFO
#endif
        } else {

          snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
          // check if allocation was successful
          if(snapshot_buf == nullptr) {
            ei_printf("ERR: Failed to allocate snapshot buffer!\n");
            delay(5000);
            return res;
          }
          out_len = fb->width * fb->height * 3;
          out_width = fb->width;
          out_height = fb->height;
          // TODO: it crashes here after a couple of loops
          // there also seems to be a memory leak, as the free heap keeps decreasing
          ei_printf("free heap: %d\n", esp_get_free_heap_size());
          ei_printf("fb->len: %d\n", fb->len);
          UBaseType_t free_bytes = uxTaskGetStackHighWaterMark(NULL);
          ei_printf("httpd handler stack high water mark: %u bytes\n", free_bytes);
          bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
          esp_camera_fb_return(fb);
          if(!converted){
            ei_printf("Conversion failed\n");
            return false;
          } else {
            fr_ready = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO

            fb_data_t rfb;
            rfb.width = out_width;
            rfb.height = out_height;
            rfb.data = snapshot_buf;
            rfb.bytes_per_pixel = 3;
            rfb.format = FB_BGR888;
            bool do_resize = false;
            if ((out_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
              || (out_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
              do_resize = true;
            }

            if (do_resize) {
              ei::image::processing::crop_and_interpolate_rgb888(
              snapshot_buf,
              EI_CAMERA_RAW_FRAME_BUFFER_COLS,
              EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
              snapshot_buf,
              out_width,
              out_height);
            }
            ei_impulse_result_t result = { 0 };

            EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
            if (err != EI_IMPULSE_OK) {
                ei_printf("ERR: Failed to run classifier (%d)\n", err);
                delay(5000);
            }

#if CONFIG_ESP_CAR_DETECT_ENABLED
            fr_car = esp_timer_get_time(); // ARDUHAL_LOG_LEVEL_INFO
            fr_recognize = fr_car; // ARDUHAL_LOG_LEVEL_INFO
#endif

            if (result.bounding_boxes_count > 0) {
              detected = true; // ARDUHAL_LOG_LEVEL_INFO
              draw_car_boxes(&rfb, &result);
            }
            s = fmt2jpg(snapshot_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len);
            free(snapshot_buf);
            if (!s) {
              log_e("fmt2jpg failed");
              res = ESP_FAIL;
            }
#if CONFIG_ESP_CAR_DETECT_ENABLED
            fr_encode = esp_timer_get_time();  // ARDUHAL_LOG_LEVEL_INFO
#endif
          }
        }
      }
#endif
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      free(_jpg_buf);
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      log_e("Send frame failed");
      break;
    }
    int64_t fr_end = esp_timer_get_time();

#if CONFIG_ESP_CAR_DETECT_ENABLED
    int64_t ready_time = (fr_ready - fr_start) / 1000; // ARDUHAL_LOG_LEVEL_INFO
    int64_t car_time = (fr_car - fr_ready) / 1000; // ARDUHAL_LOG_LEVEL_INFO
    int64_t recognize_time = (fr_recognize - fr_car) / 1000; // ARDUHAL_LOG_LEVEL_INFO
    int64_t encode_time = (fr_encode - fr_recognize) / 1000; // ARDUHAL_LOG_LEVEL_INFO
    int64_t process_time = (fr_encode - fr_start) / 1000; // ARDUHAL_LOG_LEVEL_INFO
#endif

    int64_t frame_time = fr_end - last_frame;
    frame_time /= 1000;
    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time); // ARDUHAL_LOG_LEVEL_INFO
    log_i(
      "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)"
#if CONFIG_ESP_CAR_DETECT_ENABLED
      ", %u+%u+%u+%u=%u %s%d"
#endif
      ,
      (uint32_t)(_jpg_buf_len), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time, avg_frame_time, 1000.0 / avg_frame_time
#if CONFIG_ESP_CAR_DETECT_ENABLED
      ,
      (uint32_t)ready_time, (uint32_t)car_time, (uint32_t)recognize_time, (uint32_t)encode_time, (uint32_t)process_time, (detected) ? "DETECTED " : "", 0
#endif
    );
  }

  return res;
}

static esp_err_t parse_get(httpd_req_t *req, char **obuf) {
  char *buf = NULL;
  size_t buf_len = 0;

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char *)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      *obuf = buf;
      return ESP_OK;
    }
    free(buf);
  }
  httpd_resp_send_404(req);
  return ESP_FAIL;
}

static esp_err_t cmd_handler(httpd_req_t *req) {
  char *buf = NULL;
  char variable[32];
  char value[32];

  if (parse_get(req, &buf) != ESP_OK) {
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) != ESP_OK || httpd_query_key_value(buf, "val", value, sizeof(value)) != ESP_OK) {
    free(buf);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(buf);

  int val = atoi(value);
  log_i("%s = %d", variable, val);
  sensor_t *s = esp_camera_sensor_get();
  int res = 0;

  if (!strcmp(variable, "framesize")) {
    if (s->pixformat == PIXFORMAT_JPEG) {
      res = s->set_framesize(s, (framesize_t)val);
    }
  } else if (!strcmp(variable, "quality")) {
    res = s->set_quality(s, val);
  } else if (!strcmp(variable, "contrast")) {
    res = s->set_contrast(s, val);
  } else if (!strcmp(variable, "brightness")) {
    res = s->set_brightness(s, val);
  } else if (!strcmp(variable, "saturation")) {
    res = s->set_saturation(s, val);
  } else if (!strcmp(variable, "gainceiling")) {
    res = s->set_gainceiling(s, (gainceiling_t)val);
  } else if (!strcmp(variable, "colorbar")) {
    res = s->set_colorbar(s, val);
  } else if (!strcmp(variable, "awb")) {
    res = s->set_whitebal(s, val);
  } else if (!strcmp(variable, "agc")) {
    res = s->set_gain_ctrl(s, val);
  } else if (!strcmp(variable, "aec")) {
    res = s->set_exposure_ctrl(s, val);
  } else if (!strcmp(variable, "hmirror")) {
    res = s->set_hmirror(s, val);
  } else if (!strcmp(variable, "vflip")) {
    res = s->set_vflip(s, val);
  } else if (!strcmp(variable, "awb_gain")) {
    res = s->set_awb_gain(s, val);
  } else if (!strcmp(variable, "agc_gain")) {
    res = s->set_agc_gain(s, val);
  } else if (!strcmp(variable, "aec_value")) {
    res = s->set_aec_value(s, val);
  } else if (!strcmp(variable, "aec2")) {
    res = s->set_aec2(s, val);
  } else if (!strcmp(variable, "dcw")) {
    res = s->set_dcw(s, val);
  } else if (!strcmp(variable, "bpc")) {
    res = s->set_bpc(s, val);
  } else if (!strcmp(variable, "wpc")) {
    res = s->set_wpc(s, val);
  } else if (!strcmp(variable, "raw_gma")) {
    res = s->set_raw_gma(s, val);
  } else if (!strcmp(variable, "lenc")) {
    res = s->set_lenc(s, val);
  } else if (!strcmp(variable, "special_effect")) {
    res = s->set_special_effect(s, val);
  } else if (!strcmp(variable, "wb_mode")) {
    res = s->set_wb_mode(s, val);
  } else if (!strcmp(variable, "ae_level")) {
    res = s->set_ae_level(s, val);
  }

#if CONFIG_ESP_CAR_DETECT_ENABLED
  else if (!strcmp(variable, "face_detect")) {
    detection_enabled = val;
  }
#endif
  else {
    log_i("Unknown command: %s", variable);
    res = -1;
  }

  if (res < 0) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static int print_reg(char *p, sensor_t *s, uint16_t reg, uint32_t mask) {
  return sprintf(p, "\"0x%x\":%u,", reg, s->get_reg(s, reg, mask));
}

static esp_err_t status_handler(httpd_req_t *req) {
  static char json_response[1024];

  sensor_t *s = esp_camera_sensor_get();
  char *p = json_response;
  *p++ = '{';

  if (s->id.PID == OV5640_PID || s->id.PID == OV3660_PID) {
    for (int reg = 0x3400; reg < 0x3406; reg += 2) {
      p += print_reg(p, s, reg, 0xFFF);  //12 bit
    }
    p += print_reg(p, s, 0x3406, 0xFF);

    p += print_reg(p, s, 0x3500, 0xFFFF0);  //16 bit
    p += print_reg(p, s, 0x3503, 0xFF);
    p += print_reg(p, s, 0x350a, 0x3FF);   //10 bit
    p += print_reg(p, s, 0x350c, 0xFFFF);  //16 bit

    for (int reg = 0x5480; reg <= 0x5490; reg++) {
      p += print_reg(p, s, reg, 0xFF);
    }

    for (int reg = 0x5380; reg <= 0x538b; reg++) {
      p += print_reg(p, s, reg, 0xFF);
    }

    for (int reg = 0x5580; reg < 0x558a; reg++) {
      p += print_reg(p, s, reg, 0xFF);
    }
    p += print_reg(p, s, 0x558a, 0x1FF);  //9 bit
  } else if (s->id.PID == OV2640_PID) {
    p += print_reg(p, s, 0xd3, 0xFF);
    p += print_reg(p, s, 0x111, 0xFF);
    p += print_reg(p, s, 0x132, 0xFF);
  }

  p += sprintf(p, "\"xclk\":%u,", s->xclk_freq_hz / 1000000);
  p += sprintf(p, "\"pixformat\":%u,", s->pixformat);
  p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
  p += sprintf(p, "\"quality\":%u,", s->status.quality);
  p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
  p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
  p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
  p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
  p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
  p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
  p += sprintf(p, "\"awb\":%u,", s->status.awb);
  p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
  p += sprintf(p, "\"aec\":%u,", s->status.aec);
  p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
  p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
  p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
  p += sprintf(p, "\"agc\":%u,", s->status.agc);
  p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
  p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
  p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
  p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
  p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
  p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
  p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
  p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
  p += sprintf(p, "\"colorbar\":%u", s->status.colorbar);
  p += sprintf(p, ",\"led_intensity\":%d", -1);
#if CONFIG_ESP_CAR_DETECT_ENABLED
  p += sprintf(p, ",\"face_detect\":%u", detection_enabled);
#endif
  *p++ = '}';
  *p++ = 0;
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t xclk_handler(httpd_req_t *req) {
  char *buf = NULL;
  char _xclk[32];

  if (parse_get(req, &buf) != ESP_OK) {
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "xclk", _xclk, sizeof(_xclk)) != ESP_OK) {
    free(buf);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(buf);

  int xclk = atoi(_xclk);
  log_i("Set XCLK: %d MHz", xclk);

  sensor_t *s = esp_camera_sensor_get();
  int res = s->set_xclk(s, LEDC_TIMER_0, xclk);
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t reg_handler(httpd_req_t *req) {
  char *buf = NULL;
  char _reg[32];
  char _mask[32];
  char _val[32];

  if (parse_get(req, &buf) != ESP_OK) {
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "reg", _reg, sizeof(_reg)) != ESP_OK || httpd_query_key_value(buf, "mask", _mask, sizeof(_mask)) != ESP_OK
      || httpd_query_key_value(buf, "val", _val, sizeof(_val)) != ESP_OK) {
    free(buf);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(buf);

  int reg = atoi(_reg);
  int mask = atoi(_mask);
  int val = atoi(_val);
  log_i("Set Register: reg: 0x%02x, mask: 0x%02x, value: 0x%02x", reg, mask, val);

  sensor_t *s = esp_camera_sensor_get();
  int res = s->set_reg(s, reg, mask, val);
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t greg_handler(httpd_req_t *req) {
  char *buf = NULL;
  char _reg[32];
  char _mask[32];

  if (parse_get(req, &buf) != ESP_OK) {
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "reg", _reg, sizeof(_reg)) != ESP_OK || httpd_query_key_value(buf, "mask", _mask, sizeof(_mask)) != ESP_OK) {
    free(buf);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(buf);

  int reg = atoi(_reg);
  int mask = atoi(_mask);
  sensor_t *s = esp_camera_sensor_get();
  int res = s->get_reg(s, reg, mask);
  if (res < 0) {
    return httpd_resp_send_500(req);
  }
  log_i("Get Register: reg: 0x%02x, mask: 0x%02x, value: 0x%02x", reg, mask, res);

  char buffer[20];
  const char *val = itoa(res, buffer, 10);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, val, strlen(val));
}

static int parse_get_var(char *buf, const char *key, int def) {
  char _int[16];
  if (httpd_query_key_value(buf, key, _int, sizeof(_int)) != ESP_OK) {
    return def;
  }
  return atoi(_int);
}

static esp_err_t pll_handler(httpd_req_t *req) {
  char *buf = NULL;

  if (parse_get(req, &buf) != ESP_OK) {
    return ESP_FAIL;
  }

  int bypass = parse_get_var(buf, "bypass", 0);
  int mul = parse_get_var(buf, "mul", 0);
  int sys = parse_get_var(buf, "sys", 0);
  int root = parse_get_var(buf, "root", 0);
  int pre = parse_get_var(buf, "pre", 0);
  int seld5 = parse_get_var(buf, "seld5", 0);
  int pclken = parse_get_var(buf, "pclken", 0);
  int pclk = parse_get_var(buf, "pclk", 0);
  free(buf);

  log_i("Set Pll: bypass: %d, mul: %d, sys: %d, root: %d, pre: %d, seld5: %d, pclken: %d, pclk: %d", bypass, mul, sys, root, pre, seld5, pclken, pclk);
  sensor_t *s = esp_camera_sensor_get();
  int res = s->set_pll(s, bypass, mul, sys, root, pre, seld5, pclken, pclk);
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t win_handler(httpd_req_t *req) {
  char *buf = NULL;

  if (parse_get(req, &buf) != ESP_OK) {
    return ESP_FAIL;
  }

  int startX = parse_get_var(buf, "sx", 0);
  int startY = parse_get_var(buf, "sy", 0);
  int endX = parse_get_var(buf, "ex", 0);
  int endY = parse_get_var(buf, "ey", 0);
  int offsetX = parse_get_var(buf, "offx", 0);
  int offsetY = parse_get_var(buf, "offy", 0);
  int totalX = parse_get_var(buf, "tx", 0);
  int totalY = parse_get_var(buf, "ty", 0);  // codespell:ignore totaly
  int outputX = parse_get_var(buf, "ox", 0);
  int outputY = parse_get_var(buf, "oy", 0);
  bool scale = parse_get_var(buf, "scale", 0) == 1;
  bool binning = parse_get_var(buf, "binning", 0) == 1;
  free(buf);

  log_i(
    "Set Window: Start: %d %d, End: %d %d, Offset: %d %d, Total: %d %d, Output: %d %d, Scale: %u, Binning: %u", startX, startY, endX, endY, offsetX, offsetY,
    totalX, totalY, outputX, outputY, scale, binning  // codespell:ignore totaly
  );
  sensor_t *s = esp_camera_sensor_get();
  int res = s->set_res_raw(s, startX, startY, endX, endY, offsetX, offsetY, totalX, totalY, outputX, outputY, scale, binning);  // codespell:ignore totaly
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    if (s->id.PID == OV3660_PID) {
      return httpd_resp_send(req, (const char *)index_ov3660_html_gz, index_ov3660_html_gz_len);
    } else if (s->id.PID == OV5640_PID) {
      return httpd_resp_send(req, (const char *)index_ov5640_html_gz, index_ov5640_html_gz_len);
    } else {
      return httpd_resp_send(req, (const char *)index_ov2640_html_gz, index_ov2640_html_gz_len);
    }
  } else {
    log_e("Camera sensor not found");
    return httpd_resp_send_500(req);
  }
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;
  // bump from the default (~4 KB) up to, say, 16 KB:
  config.stack_size = 32 * 1024;

  httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t status_uri = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = status_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t cmd_uri = {
    .uri = "/control",
    .method = HTTP_GET,
    .handler = cmd_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t bmp_uri = {
    .uri = "/bmp",
    .method = HTTP_GET,
    .handler = bmp_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t xclk_uri = {
    .uri = "/xclk",
    .method = HTTP_GET,
    .handler = xclk_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t reg_uri = {
    .uri = "/reg",
    .method = HTTP_GET,
    .handler = reg_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t greg_uri = {
    .uri = "/greg",
    .method = HTTP_GET,
    .handler = greg_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t pll_uri = {
    .uri = "/pll",
    .method = HTTP_GET,
    .handler = pll_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t win_uri = {
    .uri = "/resolution",
    .method = HTTP_GET,
    .handler = win_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  ra_filter_init(&ra_filter, 20);

  log_i("Starting web server on port: '%d'", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &bmp_uri);

    httpd_register_uri_handler(camera_httpd, &xclk_uri);
    httpd_register_uri_handler(camera_httpd, &reg_uri);
    httpd_register_uri_handler(camera_httpd, &greg_uri);
    httpd_register_uri_handler(camera_httpd, &pll_uri);
    httpd_register_uri_handler(camera_httpd, &win_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  log_i("Starting stream server on port: '%d'", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setupLedFlash(int pin) {
  log_i("LED flash is disabled -> CONFIG_LED_ILLUMINATOR_ENABLED = 0");
}
