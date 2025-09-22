idf_component_register(
  SRCS
    "display_sh86001.c"
    "main.c"
    "sdmmc_driver.c"
    "lvgl_panel.c"
    "image_loader.cc"
    "axp2101_driver.cc"
    "font/FontAwesome30.c"
  INCLUDE_DIRS "."
)

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE
  )
