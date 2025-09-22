
#include "image_loader.h"
#include <JPEGDEC.h>
#include <errno.h>
#include "esp_system.h"
#include "sdmmc_driver.h"

#define JPG_FILE_BUFFER_SIZE 200000
#define JPG_IMAGE_BUFFER_SIZE (400 * 450)
#define MAXOUTPUTSIZE 103

static const char* TAG = "IMAGE";

static int32_t current_image_id_ = 0;

FILE* SdmmcOpenFile(const char* file_path) {
  FILE* fp = fopen(file_path, "rb");
  if (fp == NULL) {
    ESP_LOGE(TAG, "Failed to open file %s. Error: %s", file_path, strerror(errno));
  }
  return fp;
}

static FILE* fp_timebg_ = NULL;
static bool OpenJpgImage(const char* file_path) {
  if (fp_timebg_ != NULL) {
    fclose(fp_timebg_);
  }
  fp_timebg_ = SdmmcOpenFile(file_path);
  return fp_timebg_ != NULL;
}
static size_t ReadTimeBGFrameToBuffer(uint8_t* data_buffer) {
  if (data_buffer == NULL) return 0;
  if (fp_timebg_ == NULL) return 0;

  fseek(fp_timebg_, 0, SEEK_END);
  long filesize = ftell(fp_timebg_);
  if (fseek(fp_timebg_, 0, SEEK_SET) != 0) {
    ESP_LOGE(TAG, "[SD ERROR] Failed to seek 0 in file");
    return 0;
  }

  size_t read_bytes = fread((char*)data_buffer, 1, filesize, fp_timebg_);
  fclose(fp_timebg_);
  fp_timebg_ = NULL;
  if (read_bytes != filesize) {
    ESP_LOGE(TAG, "[SD ERROR] Failed to read %lu %u in file", filesize, read_bytes);
    return 0;
  }
  return read_bytes;
}

// Static instance of the JPEGDEC structure. It requires about
// 17.5K of RAM. You can allocate it dynamically too. Internally it
// does not allocate or free any memory; all memory management decisions
// are left to you
JPEGDEC jpeg_decoder_;

static uint8_t* jpg_file_buffer_;
static uint16_t* jpg_image_buffer_;
static uint16_t jpg_width_ = 0, jpg_height_ = 0;
static uint16_t* jpg_image_buffer_read_tmp_ = NULL;
static int jpg_to_memory_callback(JPEGDRAW* pDraw) {
  if (!jpg_image_buffer_read_tmp_) return 0;
  for (int j = 0; j < pDraw->iHeight; j++) {
    memcpy(jpg_image_buffer_read_tmp_ + (pDraw->y + j) * jpg_width_ + pDraw->x,
           pDraw->pPixels + j * pDraw->iWidth, pDraw->iWidth * 2);
  }
  return 1;  // returning true (1) tells JPEGDEC to continue decoding. Returning
             // false (0) would quit decoding immediately.
}

bool ReadJpgBufferInternal(uint32_t file_size, uint16_t* image_buffer, bool flag_raw) {
  jpg_image_buffer_read_tmp_ = image_buffer;
  bool ret = jpeg_decoder_.openRAM(jpg_file_buffer_, file_size, jpg_to_memory_callback);
  if (ret) {
    jpg_width_ = jpeg_decoder_.getWidth();
    jpg_height_ = jpeg_decoder_.getHeight();

    jpeg_decoder_.setMaxOutputSize(MAXOUTPUTSIZE);
    // RGB565_LITTLE_ENDIAN ； RGB565_BIG_ENDIAN; RGB8888
    jpeg_decoder_.setPixelType(RGB565_BIG_ENDIAN);

    // need to use JPEG_USES_DMA, other wise the image will have glitch
    if (!jpeg_decoder_.decode(0, 0, JPEG_USES_DMA)) {
      ret = false;
    }
    jpeg_decoder_.close();
  }
  jpg_image_buffer_read_tmp_ = NULL;
  return ret;
}

bool LoadImageJPG(char* image_path, uint16_t* jpg_image_buffer) {
  if (!OpenJpgImage(image_path)) {
    ESP_LOGE(TAG, "[MEME] Failed to open image file for %s", image_path);
    return false;
  }
  // load the jpg image
  size_t file_length = ReadTimeBGFrameToBuffer(jpg_file_buffer_);
  if (file_length == 0) {
    return false;
  }
  return ReadJpgBufferInternal(file_length, jpg_image_buffer, true);
}

bool LoadScreenSizeImageJPG(char* image_path) {
  return LoadImageJPG(image_path, jpg_image_buffer_);
}

int MemeImageWidth() { return jpg_width_; }

int MemeImageHeight() { return jpg_height_; }

const uint8_t* MemeGetImageBuffer() { return (const uint8_t*)jpg_image_buffer_; }

static uint16_t image_count_;
static char image_paths_[MAX_NUM_IMAGE][META_FILE_MAX_WIDTH];

void InitializeImageLoader() {
  ESP_LOGI(TAG, "Initialize image loader.");
  current_image_id_ = ParameterGetCurrentTab();

  // allocate memory for buffers
  jpg_file_buffer_ =
      (uint8_t*)heap_caps_malloc(JPG_FILE_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!jpg_file_buffer_) {
    ESP_LOGE(TAG, "[MEME] Failed to load allocate jpg file buffer!!!");
  }

  jpg_image_buffer_ =
      (uint16_t*)heap_caps_malloc(JPG_IMAGE_BUFFER_SIZE * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!jpg_image_buffer_) {
    ESP_LOGE(TAG, "[MEME] Failed to load allocate jpg image data buffer!!!");
  }

  // count stereo memes
  image_count_ = ReadMetaFileLines("/sd/prod/meta.txt", image_paths_, MAX_NUM_IMAGE);
  ESP_LOGI(TAG, "[MEME] load %d images\n", image_count_);
}

uint16_t ReadMetaFileLines(const char* file_path, char meta_lines[][META_FILE_MAX_WIDTH],
                           uint16_t max_num_lines) {
  FILE* fp = SdmmcOpenFile(file_path);
  if (fp == NULL) return 0;

  uint16_t idx = 0;
  while (fgets(meta_lines[idx], META_FILE_MAX_WIDTH, fp)) {
    // Remove newline character if present
    meta_lines[idx][strcspn(meta_lines[idx], "\r\n")] = 0;
    // DMLOG("  - Line: %s\n", meta_lines[idx]);
    idx++;
    if (idx >= max_num_lines) {
      ESP_LOGE(TAG, "[SD ERROR] %s reach maximal number of chars %d", file_path, idx);
      break;
    }
  }
  fclose(fp);
  return idx;
}

bool LoadNextImageJPG() {
  if (image_count_ == 0) return false;
  // uint16_t image_id = current_image_id_;
  // while (image_id == current_image_id_) {
  //   image_id = esp_random() % image_count_;
  // }
  // current_image_id_ = image_id;

  int32_t image_id = ++current_image_id_ % image_count_;
  current_image_id_ = image_id;
  ParameterSetCurrentTab(current_image_id_);

  static char tmp_file_path[257];
  snprintf(tmp_file_path, 257, "/sd/prod/%s", image_paths_[image_id]);
  // ESP_LOGI(TAG, "load image %s", tmp_file_path);

  return LoadImageJPG(tmp_file_path, jpg_image_buffer_);
}
