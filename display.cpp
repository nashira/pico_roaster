#include "display.h"
#include "temperature.h"
#include "sensors.h"
#include <Wire.h>
// #define U8x8_DO_NOT_SET_WIRE_CLOCK
#include <U8g2lib.h>

#define FS 18

void display_draw(void *data);
static task_t display_draw_task = {
    .execute = display_draw,
    .period_us = -ftous(10)
};

static U8G2_SH1106_128X64_NONAME_F_2ND_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
static char str[8];

void display_init() {
  Wire1.setSDA(18);
  Wire1.setSCL(19);
  Wire1.setClock(2'000'000);
  display.begin();
  display.setFont(u8g2_font_helvB18_tr);

  core0_schedule(0, &display_draw_task);
}

void display_draw(void *data) {
  // Serial.println("display_draw");

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

  int line2_y =  y1 + FS * 2 + 4;
  display.setCursor(x1, line2_y);
  display.print(temperature2(), 1);

  dtostrf(temperature2_dx(), 1, 1, str);
  w = display.getStrWidth(str);
  display.setCursor(x2-w, line2_y);
  display.print(str);

  int line3_y =  y2;
  display.setCursor(x1, line3_y);
  display.print(sensors_voltage(), 1);
  display.print("v");

  dtostrf(sensors_angle(), 1, 1, str);
  strcat(str, "a");
  w = display.getStrWidth(str);
  display.setCursor(x2-w, line3_y);
  display.print(str);

  display.sendBuffer();
}
