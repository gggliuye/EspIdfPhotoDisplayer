

#include "display_sh86001.h"
#include "esp_random.h"
#include "sdmmc_driver.h"

#define META_FILE_MAX_WIDTH 20
#define MAX_NUM_IMAGE 1000

#ifdef __cplusplus
extern "C" {
#endif

void InitializeImageLoader();
bool LoadImageJPG(char* image_path, uint16_t* jpg_image_buffer);
bool LoadScreenSizeImageJPG(char* image_path);
bool LoadNextImageJPG();

int MemeImageWidth();
int MemeImageHeight();
const uint8_t* MemeGetImageBuffer();

uint16_t ReadMetaFileLines(const char* file_path, char meta_lines[][META_FILE_MAX_WIDTH],
                           uint16_t max_num_lines);

#ifdef __cplusplus
}
#endif
