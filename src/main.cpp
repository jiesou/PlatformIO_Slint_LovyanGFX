#include <slint-lgfx.h> // Must be the first included header
#include <Arduino.h>
#include "lgfx.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app-window.h" // Generated header in .pio/build/[env]/Slint_LovyanGFX_generated/app-window.h

// There are lint errors? try Ctrl+Shift+P -> PlatformIO: Rebuild IntelliSense Index
void slint_task(void *pvParameters)
{
  // Similar to: https://docs.slint.dev/latest/docs/cpp/mcu/esp_idf
  slint_esp_init(SlintPlatformConfiguration{
      .size = slint::PhysicalSize({(uint32_t)gfx.width(), (uint32_t)gfx.height()}),
      // DONT need the
      // .panel_handle = panel_handle,
      // .touch_handle = touch_handle,
      // but need LGFX instance instead
      .gfx = &gfx,
      .byte_swap = true});

  auto ui = MainWindow::create();
  ui->run();

  // C++ bindings see: https://docs.slint.dev/latest/docs/cpp/overview
  // slint::run_event_loop();
  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("=== Booting ===");

  gfx.init();
  gfx.setRotation(1);

  // Simple test with LovyanGFX only
  gfx.fillScreen(TFT_RED);
  delay(300);
  gfx.fillScreen(TFT_GREEN);
  delay(300);
  gfx.fillScreen(TFT_BLUE);
  delay(300);

  // Create a task for Slint with a larger stack in FreeRTOS
  xTaskCreatePinnedToCore(
      slint_task,
      "slint_task",
      8 * 1024, // 8KB stack, need more if UI is complex
      NULL, // Parameters
      1, // Priority
      NULL, // Task handle
      1 // Core 1
  );
}

void loop()
{
  // Nothing to do here, Slint runs in its own task
}
