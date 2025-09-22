
#include "lvgl_panel.h"

static const char* TAG = "LVGL";

static lv_obj_t* stereo_image_ = NULL;
static lv_obj_t* battery_label_ = NULL;
static lv_img_dsc_t background_img_dsc_;
static lv_style_t style_icon;
static int64_t last_change_time_ = 0;
static lv_timer_t* auto_step_timer_ = NULL;

LV_FONT_DECLARE(FontAwesome30);

static void UpdateImage() {
  LoadNextImageJPG();
  background_img_dsc_.header.w = MemeImageWidth();
  background_img_dsc_.header.h = MemeImageHeight();
  background_img_dsc_.data_size = MemeImageWidth() * MemeImageHeight() * 2;
  lv_obj_invalidate(stereo_image_);
  last_change_time_ = esp_timer_get_time();
}

static void event_handler_view_change(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_CLICKED) return;
  lv_dir_t gesture = lv_indev_get_gesture_dir(lv_indev_get_act());
  if (gesture == LV_DIR_LEFT || gesture == LV_DIR_RIGHT || gesture == LV_DIR_TOP ||
      gesture == LV_DIR_BOTTOM) {
    // Ignore left/right gesture-induced clicks
    return;
  }

  // to avoid false click
  int64_t current = esp_timer_get_time();
  static int64_t last_click = 0;
  if (current - last_click < 200000) {
    return;
  }
  last_click = current;

  UpdateImage();
}

static int brightness = 128;  // Initial brightness (50%)

static void screen_gesture_event_cb(lv_event_t* e) {
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  switch (dir) {
    case LV_DIR_LEFT:
      break;
    case LV_DIR_RIGHT:
      break;
    case LV_DIR_TOP:
      brightness += 5;
      if (brightness > 255) brightness = 255;
      lcd_set_brightness((uint8_t)brightness);
      break;
    case LV_DIR_BOTTOM:
      brightness -= 5;
      if (brightness < 0) brightness = 0;
      lcd_set_brightness((uint8_t)brightness);
      break;
  }
}

// full    3/4     1/2   1/4    low    charge
// 0xf240,0xf241,0xf242,0xf243,0xe0b0,0xf376
static bool show_charging_ = false;
void dataupdate_lvgl_tick(lv_timer_t* t) {
  // update battery
  int32_t battery = GetBatteryPercent();
  bool charging = IsCharging();
  if (charging) {
    lv_obj_set_style_text_color(battery_label_, lv_color_hex(0x87ed9d), 0);
    show_charging_ = !show_charging_;
    if (show_charging_) {
      lv_label_set_text(battery_label_, "\xEF\x8D\xB6");
    }
  } else {
    lv_obj_set_style_text_color(battery_label_, lv_color_hex(0xb4d2d4), 0);
    show_charging_ = false;
  }
  if (!show_charging_) {
    if (battery > 90) {
      lv_label_set_text(battery_label_, "\xEF\x89\x80");
    } else if (battery > 70) {
      lv_label_set_text(battery_label_, "\xEF\x89\x81");
    } else if (battery > 40) {
      lv_label_set_text(battery_label_, "\xEF\x89\x82");
    } else if (battery > 10) {
      lv_label_set_text(battery_label_, "\xEF\x89\x83");
    } else {
      lv_label_set_text(battery_label_, "\xEE\x82\xB0");
    }
  }
  // ESP_LOGI(TAG, "bat: %d, charge: %d", battery, charging);

  // add a loop to keep loading new image
  PowerLoop();
  int64_t boottime_ms = esp_timer_get_time();
  if (boottime_ms - last_change_time_ > 5000000) {
    UpdateImage();
  }
}

void CreateLvglPanel() {
  lv_style_init(&style_icon);
  lv_style_set_text_font(&style_icon, &FontAwesome30);
  lv_style_set_text_color(&style_icon, lv_color_hex(0xb4d2d4));
  lv_style_set_text_opa(&style_icon, LV_OPA_70);  // 半透明文字

  // get the current screen
  lv_obj_t* current_screen = lv_scr_act();
  lv_obj_add_event_cb(current_screen, screen_gesture_event_cb, LV_EVENT_GESTURE, NULL);

  // create image holder for the stereo meme
  stereo_image_ = lv_img_create(current_screen);
  lv_obj_center(stereo_image_);
  lv_obj_set_size(stereo_image_, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
  lv_obj_add_flag(stereo_image_, LV_OBJ_FLAG_CLICKABLE);
  // lv_obj_add_flag(stereo_image_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(stereo_image_, event_handler_view_change, LV_EVENT_CLICKED, NULL);

  if (LoadNextImageJPG()) {
    background_img_dsc_.header.always_zero = 0;
    background_img_dsc_.header.w = MemeImageWidth();
    background_img_dsc_.header.h = MemeImageHeight();
    background_img_dsc_.header.cf = LV_IMG_CF_TRUE_COLOR;
    background_img_dsc_.data_size = MemeImageWidth() * MemeImageHeight() * 2;
    background_img_dsc_.data = MemeGetImageBuffer();
    lv_img_set_src(stereo_image_, &background_img_dsc_);
  }

  // add battery label
  battery_label_ = lv_label_create(current_screen);
  lv_obj_add_style(battery_label_, &style_icon, 0);
  lv_obj_align(battery_label_, LV_ALIGN_CENTER, -EXAMPLE_LCD_H_RES * 0.4,
               -EXAMPLE_LCD_V_RES * 0.45);
  lv_label_set_text(battery_label_, "\xEF\x89\x80");

  last_change_time_ = esp_timer_get_time();
  auto_step_timer_ = lv_timer_create(dataupdate_lvgl_tick, 500, NULL);
}
