#include "display.h"
#include "temperature.h"
#include <Wire.h>
#include <U8g2lib.h>

#define FS 18

void display_draw(void *data);
static task_t display_draw_task = {
    .execute = display_draw,
    .period_us = -ftous(10)
};

static U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
static char str[8];

void display_init() {
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.setClock(2'000'000);
  display.begin();
  display.setFont(u8g2_font_helvB18_tr);

  // core0_schedule(0, &display_draw_task);
}

void display_draw(void *data) {
  // Serial.println("display_draw");
  return;
  int o = (millis() / 10000) % 3;
  int x1 = o, x2 = 126 + o;
  int y1 = o, y2 = 62 + o;

  display.clearBuffer();

  display.setCursor(x1, y1 + FS);
  display.print(temperature1(), 1);

  dtostrf(temperature1_dx(), 1, 1, str);
  int w = display.getStrWidth(str);
  display.setCursor(x2-w, y1 + FS);
  display.print(str);

  display.setCursor(x1,y2);
  display.print(temperature2(), 1);

  dtostrf(temperature2_dx(), 1, 1, str);
  w = display.getStrWidth(str);
  display.setCursor(x2-w, y2);
  display.print(str);

  display.sendBuffer();
}
