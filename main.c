//#include <stdlib.h>
//#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "PCF8575.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
// SPI Defines
#define SPI_PORT spi0
#define SPI_MISO  16 // GP16 RX -> DOUT
#define SPI_CS    17 // GP17
#define SPI_SCK   18 // GP18
#define SPI_MOSI  19 // GP19 TX -> DIN
// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 4 // GP4 (6)
#define I2C_SCL 5 // GP5 (7)
// pins for knob
#define PIN_KNOB_0 6 // KNOB 0, The steering wheel has 0.5 a turn of travel
#define PIN_KNOB_1 7 // KNOB 1, The steering wheel has 1.0 a turn of travel
#define PIN_KNOB_2 8 // KNOB 2, The steering wheel has 1.5 a turn of travel
#define PIN_KNOB_3 9 // KNOB 3, RAW data, the steering wheel has 2.0 a turn of travel
// wheel values for corrections
#define WHEEL_CENTER 2150 // values obtained from jscal -c /dev/input/js0
#define WHEEL_PM_VALUE_1 580 // for 0.5 rotation
#define WHEEL_PM_VALUE_2 1120 // for 1 rotation
#define WHEEL_PM_VALUE_3 1660 // for 1.5 rotation
/* blink pattern */
enum  {
  BLINK_NOT_MOUNTED = 250,// device not mounted
  BLINK_MOUNTED = 1000,   // device mounted
  BLINK_SUSPENDED = 2500, // device is suspended
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
uint64_t loop_time = 0, delta_time = 0;

PCF8575 wheelbtns;

void led_blinking_task(void);
void hid_task(void);

/*------------- MAIN -------------*/
int main(void) {
  set_sys_clock_48mhz();
  board_init();
// init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb) {
    board_init_after_tusb();
  }
// init input pins
  gpio_init_mask( (1<<PIN_KNOB_0) | (1<<PIN_KNOB_1) |(1<<PIN_KNOB_2) |(1<<PIN_KNOB_3) );
// init spi
  spi_init(SPI_PORT, 1000000);// 100kHz
  spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(SPI_MISO, GPIO_FUNC_SPI);
  gpio_set_function(SPI_CS, GPIO_FUNC_SIO);
  gpio_set_dir(SPI_CS, GPIO_OUT);
  gpio_put(SPI_CS, 1);
  gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
  gpio_set_function(SPI_MOSI, GPIO_FUNC_SPI);
// init i2c
  i2c_init(I2C_PORT, 100000);// 100kHz
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
// initialize devices
  PCF8575_init(&wheelbtns, I2C_PORT, PCF8575_ADDR_DEF);
// main loop
  while (1) {
    tud_task(); // tinyusb device task
    led_blinking_task();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
/// HID Gamepad Protocol Report.
// size of this structure must be not great then CFG_TUD_HID_EP_BUFSIZE - size of ID
typedef struct TU_ATTR_PACKED {
  uint16_t  x;         ///< Delta x  movement of left analog-stick
  uint16_t  y;         ///< Delta y  movement of left analog-stick
  uint16_t  z;         ///< Delta z  movement of right analog-joystick
  uint16_t  rx;        ///< Delta Rz movement of right analog-joystick
  uint16_t  ry;        ///< Delta Rx movement of analog left trigger
  uint8_t hat;       ///< Buttons mask for currently pressed buttons in the DPad/hat
  uint16_t buttons;  ///< Buttons mask for currently pressed buttons
  uint8_t gears;
} hid_custom_gamepad_report_t;

uint8_t MapHat(uint8_t HatSwitch) {
  uint8_t RetVal = 0;
  switch (HatSwitch) {
  //case 0: RetVal = GAMEPAD_HAT_CENTERED; break; // uldr : center : hatButtons = B00000000
  case 1: RetVal = GAMEPAD_HAT_UP; break; // Uldr : 0 : hatButtons = B0001
  case 3: RetVal = GAMEPAD_HAT_UP_RIGHT; break; // ULdr : 45 : hatButtons = B0011
  case 2: RetVal = GAMEPAD_HAT_RIGHT; break; // uLdr : 90 : hatButtons = B0010
  case 6: RetVal = GAMEPAD_HAT_DOWN_RIGHT; break; // uLDr : 135 : hatButtons = B0110
  case 4: RetVal = GAMEPAD_HAT_DOWN; break; // ulDr : 180 : hatButtons = B0100
  case 12: RetVal = GAMEPAD_HAT_DOWN_LEFT; break; // ulDR : 225 : hatButtons = B1100
  case 8: RetVal = GAMEPAD_HAT_LEFT; break; // uldR : 270 : hatButtons = B1000
  case 9: RetVal = GAMEPAD_HAT_UP_LEFT; break; // UldR : 315 : hatButtons = B1001
  }
  return RetVal;
}

uint8_t MapGear(uint16_t rawGear) {
  uint8_t newGear;
  static uint8_t lastGear = 0;
  newGear = ((rawGear+256)>>9);// all gears should be in n*512 +-210, if not, correct this formula
  if (newGear != lastGear) {// we delay reading the new gear
    newGear = 0;
    lastGear = newGear;
  }
  return newGear;
}

// read analog inputs 0 .. 7, returns 0..4095
uint16_t read_mcp3208(uint8_t channel) {
  if (channel >= 8) return 0;
  // sending 3 bytes, figure 6-1 from datasheet
  uint8_t tx_buf[3];
  tx_buf[0] = 0x06 | (channel >> 2); 
  tx_buf[1] = channel << 6;
  tx_buf[2] = 0x00;
  uint8_t rx_buf[3] = {0, 0, 0};
  //asm volatile("nop \n nop \n nop");
  gpio_put(SPI_CS, 0);
  //asm volatile("nop \n nop \n nop");
  spi_write_read_blocking(SPI_PORT, tx_buf, rx_buf, 3);
  //asm volatile("nop \n nop \n nop");
  gpio_put(SPI_CS, 1);
  // composing 12 bits from bytes
  uint16_t result = ((rx_buf[1] & 0x0F) << 8) | rx_buf[2];
  return result;
}

uint16_t corrected_read_mcp3208(uint8_t channel, uint8_t faster_wheel) {
  uint16_t val = read_mcp3208(channel);
  if (channel != 0) { // pedals or handbrake
    if (val < 96) val = 0;
    if (val >= 4000) val = 4095;
  } else {// stering wheel
    if (faster_wheel & (1<<PIN_KNOB_0)) {
      if (val < WHEEL_CENTER - WHEEL_PM_VALUE_1) val = 0;
      else if (val >= WHEEL_CENTER + WHEEL_PM_VALUE_1) val = 4095;
      else {
        float t = val - WHEEL_CENTER + WHEEL_PM_VALUE_1;
        t *= 3903;
        t /= (2* WHEEL_PM_VALUE_1 -1);
        val = t + 96;
      }
    } else if (faster_wheel & (1<<PIN_KNOB_1)){
      if (val < WHEEL_CENTER - WHEEL_PM_VALUE_2) val = 0;
      else if (val >= WHEEL_CENTER + WHEEL_PM_VALUE_2) val = 4095;
      else {
        float t = val - WHEEL_CENTER + WHEEL_PM_VALUE_2;
        t *= 3903;
        t /= (2* WHEEL_PM_VALUE_2 -1);
        val = t + 96;
      }
    }  else if (faster_wheel & (1<<PIN_KNOB_2)){
      if (val < WHEEL_CENTER - WHEEL_PM_VALUE_3) val = 0;
      else if (val >= WHEEL_CENTER + WHEEL_PM_VALUE_3) val = 4095; 
      else {
        float t = val - WHEEL_CENTER + WHEEL_PM_VALUE_3;
        t *= 3903;
        t /= (2* WHEEL_PM_VALUE_3 -1);
        val = 96 + t;
      }
    }
  }
  return val;
}

static void send_hid_report(uint8_t report_id) {
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  hid_custom_gamepad_report_t report = {
        0//.x   = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .hat = 0, .buttons = 0
  };// buttons - 32 flags with the state of buttons 0 - 31, hat has a type uint8_t, other axis are int8_t
  uint32_t rpins = ~gpio_get_all();
  uint16_t btns = ~PCF8575_read_all(&wheelbtns);
  report.x = corrected_read_mcp3208(0, rpins );// Steering wheel position
  report.y = corrected_read_mcp3208(1, 0);// Throttle pedal input
  report.z = corrected_read_mcp3208(2, 0);// Brake pedal input
  report.rx = corrected_read_mcp3208(3, 0);// Clutch pedal input
  report.ry = corrected_read_mcp3208(4, 0);// Analog handbrake input
  uint8_t gear = MapGear(read_mcp3208(5));// Shifter gear position
  if (gear > 0) report.gears |= (1 << (gear -1));
  // you can change the order of buttons here
  if (btns & 0x1000) report.buttons |= 1;//GAMEPAD_BUTTON_A;
  if (btns & 0x2000) report.buttons |= 2;//GAMEPAD_BUTTON_B;
  if (btns & 0x4000) report.buttons |= 4;//GAMEPAD_BUTTON_Y;
  if (btns & 0x8000) report.buttons |= 8;//GAMEPAD_BUTTON_X;
  if (btns & 0x0010) report.buttons |= 16;//GAMEPAD_BUTTON_SELECT;
  if (btns & 0x0800) report.buttons |= 32;//GAMEPAD_BUTTON_START;
  if (btns & 0x0020) report.buttons |= 64;//GAMEPAD_BUTTON_TL;
  if (btns & 0x0400) report.buttons |= 128;//GAMEPAD_BUTTON_TR;
  if (btns & 0x0040) report.buttons |= 256;//GAMEPAD_BUTTON_C;
  if (btns & 0x0200) report.buttons |= 512;//GAMEPAD_BUTTON_Z;
  if (btns & 0x0080) report.buttons |= 1024;//GAMEPAD_BUTTON_THUMBL;
  if (btns & 0x0100) report.buttons |= 2048;//GAMEPAD_BUTTON_THUMBR;
  report.hat |= MapHat(btns & 0x0F); // D-PAD
  tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void) {
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;
  // Remote wakeup
  if ( tud_suspended() ) {
    // Wake up host if we are in suspend mode and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  } else {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_GAMEPAD);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
  // not Implemented
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  // not Implemented
  return 0;
}

// Invoked when received SET_REPORT control request or received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    // not Implemented
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
