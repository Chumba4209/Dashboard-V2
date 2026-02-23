#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <DHT.h>
#define DHT_PIN 27
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

static lv_color_t draw_buf[SCREEN_WIDTH * 20];
static lv_display_t *display;

TFT_eSPI tft = TFT_eSPI();

// Panel containers 
lv_obj_t *temp_panel;
lv_obj_t *humidity_panel;
lv_obj_t *chart_panel;
lv_obj_t *temp_arc;
lv_obj_t *humidity_arc;
lv_obj_t *temp_value_label;
lv_obj_t *humidity_value_label;
lv_obj_t *chart;
lv_chart_series_t *temp_series;
lv_chart_series_t *humidity_series;



void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

static uint32_t my_tick(void) {
    return millis();
}


void create_dashboard() {
    // Root screen 
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // Title label 
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "DHT22");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x333333), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);


    // Chart panel 
    chart_panel = lv_obj_create(scr);
    lv_obj_set_size(chart_panel, 320, 120);
    lv_obj_align(chart_panel, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_bg_color(chart_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(chart_panel, 1, 0);
    lv_obj_set_style_border_color(chart_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(chart_panel, 6, 0);
    lv_obj_set_style_pad_all(chart_panel, 4, 0);
    lv_obj_set_scroll_dir(chart_panel, LV_DIR_NONE);

    // Create chart inside panel
    chart = lv_chart_create(chart_panel);
    lv_obj_set_size(chart, 300, 100);
    lv_obj_center(chart);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 20);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);

    // Style the chart
    lv_obj_set_style_bg_color(chart, lv_color_white(), 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR); // hide dots

    // Temp series - green
    temp_series = lv_chart_add_series(chart, lv_color_hex(0x4CAF50), 
                                      LV_CHART_AXIS_PRIMARY_Y);

    // Humidity series - blue
    humidity_series = lv_chart_add_series(chart, lv_color_hex(0x2196F3), 
                                          LV_CHART_AXIS_PRIMARY_Y);

    // Y axis labels (static, right side of chart)
lv_obj_t *y_label_top = lv_label_create(chart_panel);
lv_label_set_text(y_label_top, "100");
lv_obj_set_style_text_font(y_label_top, &lv_font_montserrat_14, 0);
lv_obj_set_style_text_color(y_label_top, lv_color_hex(0x999999), 0);
lv_obj_align(y_label_top, LV_ALIGN_TOP_RIGHT, 5, 2);

lv_obj_t *y_label_mid = lv_label_create(chart_panel);
lv_label_set_text(y_label_mid, "50");
lv_obj_set_style_text_font(y_label_mid, &lv_font_montserrat_14, 0);
lv_obj_set_style_text_color(y_label_mid, lv_color_hex(0x999999), 0);
lv_obj_align(y_label_mid, LV_ALIGN_RIGHT_MID, 5, 0);

lv_obj_t *y_label_bot = lv_label_create(chart_panel);
lv_label_set_text(y_label_bot, "0");
lv_obj_set_style_text_font(y_label_bot, &lv_font_montserrat_14, 0);
lv_obj_set_style_text_color(y_label_bot, lv_color_hex(0x999999), 0);
lv_obj_align(y_label_bot, LV_ALIGN_BOTTOM_RIGHT, -2, -8);

// Also delete lv_obj_refresh_ext_draw_size line if you added it

    // Legend - T label (green)
    lv_obj_t *legend_t = lv_label_create(chart_panel);
    lv_label_set_text(legend_t, "T");
    lv_obj_set_style_text_color(legend_t, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_text_font(legend_t, &lv_font_montserrat_14, 0);
    lv_obj_align(legend_t, LV_ALIGN_TOP_LEFT, 4, 2);

    // Legend - H label (blue)
    lv_obj_t *legend_h = lv_label_create(chart_panel);
    lv_label_set_text(legend_h, "H");
    lv_obj_set_style_text_color(legend_h, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_text_font(legend_h, &lv_font_montserrat_14, 0);
    lv_obj_align(legend_h, LV_ALIGN_TOP_LEFT, 20, 2);
                                          
    // Pre-fill with demo data so chart is not empty
    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(chart, temp_series, 20 + (i % 5));
        lv_chart_set_next_value(chart, humidity_series, 55 + (i % 8));
    }

    // Temp panel 
    temp_panel = lv_obj_create(scr);
    lv_obj_set_size(temp_panel, 158, 100);
    lv_obj_align(temp_panel, LV_ALIGN_BOTTOM_LEFT, 1, -1);
    lv_obj_set_style_bg_color(temp_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(temp_panel, 1, 0);
    lv_obj_set_style_border_color(temp_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(temp_panel, 6, 0);
    lv_obj_set_style_pad_all(temp_panel, 4, 0);

    lv_obj_t *temp_title = lv_label_create(temp_panel);
    lv_label_set_text(temp_title, "Temperature");
    lv_obj_set_style_text_font(temp_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(temp_title, lv_color_hex(0x333333), 0);
    lv_obj_align(temp_title, LV_ALIGN_TOP_LEFT, 4, 2);

    // Temp arc gauge
    temp_arc = lv_arc_create(temp_panel);
    lv_obj_set_size(temp_arc, 70, 70);
    lv_arc_set_rotation(temp_arc, 135);
    lv_arc_set_bg_angles(temp_arc, 0, 270);
    lv_arc_set_range(temp_arc, -10, 50);
    lv_arc_set_value(temp_arc, 25);
    lv_obj_remove_style(temp_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_arc_width(temp_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(temp_arc, 8, LV_PART_MAIN);
    lv_obj_align(temp_arc, LV_ALIGN_CENTER, 10, 8);

    // Temp value label (inside arc)
    temp_value_label = lv_label_create(temp_panel);
    lv_label_set_text(temp_value_label, "25°C");
    lv_obj_set_style_text_font(temp_value_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(temp_value_label, lv_color_hex(0x333333), 0);
    lv_obj_align_to(temp_value_label, temp_arc, LV_ALIGN_CENTER, -8, 0);

    // Humidity panel 
    humidity_panel = lv_obj_create(scr);
    lv_obj_set_size(humidity_panel, 158, 100);
    lv_obj_align(humidity_panel, LV_ALIGN_BOTTOM_RIGHT, -1, -1);
    lv_obj_set_style_bg_color(humidity_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(humidity_panel, 1, 0);
    lv_obj_set_style_border_color(humidity_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(humidity_panel, 6, 0);
    lv_obj_set_style_pad_all(humidity_panel, 4, 0);

    lv_obj_t *hum_title = lv_label_create(humidity_panel);
    lv_label_set_text(hum_title, "Humidity");
    lv_obj_set_style_text_font(hum_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hum_title, lv_color_hex(0x333333), 0);
    lv_obj_align(hum_title, LV_ALIGN_TOP_LEFT, 4, 2);

    // Humidity arc gauge
    humidity_arc = lv_arc_create(humidity_panel);
    lv_obj_set_size(humidity_arc, 70, 70);
    lv_arc_set_rotation(humidity_arc, 135);
    lv_arc_set_bg_angles(humidity_arc, 0, 270);
    lv_arc_set_range(humidity_arc, 0, 100);
    lv_arc_set_value(humidity_arc, 60);
    lv_obj_remove_style(humidity_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(humidity_arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(humidity_arc, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_arc_width(humidity_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(humidity_arc, 8, LV_PART_MAIN);
    lv_obj_align(humidity_arc, LV_ALIGN_CENTER, 10, 8);

    // Humidity value label (inside arc)
    humidity_value_label = lv_label_create(humidity_panel);
    lv_label_set_text(humidity_value_label, "60%");
    lv_obj_set_style_text_font(humidity_value_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(humidity_value_label, lv_color_hex(0x333333), 0);
    lv_obj_align_to(humidity_value_label, humidity_arc, LV_ALIGN_CENTER, 0, 0);

    
}

void setup() {
    Serial.begin(115200);
    dht.begin();

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_tick_set_cb(my_tick);

    display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(display, my_disp_flush);
    lv_display_set_buffers(display, draw_buf, NULL,
                           sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    create_dashboard();
}

void loop() {
    lv_timer_handler();

    static uint32_t last_update = 0;
    if (millis() - last_update > 2000) {
        last_update = millis();

        float real_temp = dht.readTemperature();
        float real_hum  = dht.readHumidity();

        if (!isnan(real_temp) && !isnan(real_hum)) {
            // Update chart
            lv_chart_set_next_value(chart, temp_series, (int)real_temp);
            lv_chart_set_next_value(chart, humidity_series, (int)real_hum);
            lv_chart_refresh(chart);

            // Update arc gauges
            lv_arc_set_value(temp_arc, (int)real_temp);
            lv_arc_set_value(humidity_arc, (int)real_hum);

            // Update value labels
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f°C", real_temp);
            lv_label_set_text(temp_value_label, buf);
            snprintf(buf, sizeof(buf), "%.0f%%", real_hum);
            lv_label_set_text(humidity_value_label, buf);

            // Alert colors for temp gauge
            if (real_temp > 35.0 || real_temp < 10.0) {
                lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0xE53935), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
            }

            // Alert colors for humidity gauge
            if (real_hum > 80.0 || real_hum < 20.0) {
                lv_obj_set_style_arc_color(humidity_arc, lv_color_hex(0xE53935), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_arc_color(humidity_arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
            }
        }
    }

    delay(5);
}
