/* manette-steam-deck-driver.c
 *
 * Copyright (C) 2024 Alice Mikhaylenko <alicem@gnome.org>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "manette-steam-deck-driver-private.h"

#include <hidapi.h>
#include <linux/input-event-codes.h>
#include <unistd.h>

/* Heavily based on SDL steam deck code */

#define HAPTIC_INTENSITY_SYSTEM 0
#define HID_FEATURE_REPORT_BYTES 64

#define INPUT_REPORT_VERSION 0x01
#define ID_CONTROLLER_DECK_STATE 0x09

#define STICK_FLAT 2500

typedef enum {
  ID_SET_DIGITAL_MAPPINGS              = 0x80,
  ID_CLEAR_DIGITAL_MAPPINGS            = 0x81,
  ID_GET_DIGITAL_MAPPINGS              = 0x82,
  ID_GET_ATTRIBUTES_VALUES             = 0x83,
  ID_GET_ATTRIBUTE_LABEL               = 0x84,
  ID_SET_DEFAULT_DIGITAL_MAPPINGS      = 0x85,
  ID_FACTORY_RESET                     = 0x86,
  ID_SET_SETTINGS_VALUES               = 0x87,
  ID_CLEAR_SETTINGS_VALUES             = 0x88,
  ID_GET_SETTINGS_VALUES               = 0x89,
  ID_GET_SETTING_LABEL                 = 0x8A,
  ID_GET_SETTINGS_MAXS                 = 0x8B,
  ID_GET_SETTINGS_DEFAULTS             = 0x8C,
  ID_SET_CONTROLLER_MODE               = 0x8D,
  ID_LOAD_DEFAULT_SETTINGS             = 0x8E,
  ID_TRIGGER_HAPTIC_PULSE              = 0x8F,

  ID_TURN_OFF_CONTROLLER               = 0x9F,

  ID_GET_DEVICE_INFO                   = 0xA1,

  ID_CALIBRATE_TRACKPADS               = 0xA7,
  ID_RESERVED_0                        = 0xA8,
  ID_SET_SERIAL_NUMBER                 = 0xA9,
  ID_GET_TRACKPAD_CALIBRATION          = 0xAA,
  ID_GET_TRACKPAD_FACTORY_CALIBRATION  = 0xAB,
  ID_GET_TRACKPAD_RAW_DATA             = 0xAC,
  ID_ENABLE_PAIRING                    = 0xAD,
  ID_GET_STRING_ATTRIBUTE              = 0xAE,
  ID_RADIO_ERASE_RECORDS               = 0xAF,
  ID_RADIO_WRITE_RECORD                = 0xB0,
  ID_SET_DONGLE_SETTING                = 0xB1,
  ID_DONGLE_DISCONNECT_DEVICE          = 0xB2,
  ID_DONGLE_COMMIT_DEVICE              = 0xB3,
  ID_DONGLE_GET_WIRELESS_STATE         = 0xB4,
  ID_CALIBRATE_GYRO                    = 0xB5,
  ID_PLAY_AUDIO                        = 0xB6,
  ID_AUDIO_UPDATE_START                = 0xB7,
  ID_AUDIO_UPDATE_DATA                 = 0xB8,
  ID_AUDIO_UPDATE_COMPLETE             = 0xB9,
  ID_GET_CHIPID                        = 0xBA,

  ID_CALIBRATE_JOYSTICK                = 0xBF,
  ID_CALIBRATE_ANALOG_TRIGGERS         = 0xC0,
  ID_SET_AUDIO_MAPPING                 = 0xC1,
  ID_CHECK_GYRO_FW_LOAD                = 0xC2,
  ID_CALIBRATE_ANALOG                  = 0xC3,
  ID_DONGLE_GET_CONNECTED_SLOTS        = 0xC4,

  ID_RESET_IMU                         = 0xCE,

  // Deck only
  ID_TRIGGER_HAPTIC_CMD                = 0xEA,
  ID_TRIGGER_RUMBLE_CMD                = 0xEB,
} FeatureReportMessageID;

typedef enum {
  SETTING_MOUSE_SENSITIVITY,
  SETTING_MOUSE_ACCELERATION,
  SETTING_TRACKBALL_ROTATION_ANGLE,
  SETTING_HAPTIC_INTENSITY_UNUSED,
  SETTING_LEFT_GAMEPAD_STICK_ENABLED,
  SETTING_RIGHT_GAMEPAD_STICK_ENABLED,
  SETTING_USB_DEBUG_MODE,
  SETTING_LEFT_TRACKPAD_MODE,
  SETTING_RIGHT_TRACKPAD_MODE,
  SETTING_MOUSE_POINTER_ENABLED,

  // 10
  SETTING_DPAD_DEADZONE,
  SETTING_MINIMUM_MOMENTUM_VEL,
  SETTING_MOMENTUM_DECAY_AMOUNT,
  SETTING_TRACKPAD_RELATIVE_MODE_TICKS_PER_PIXEL,
  SETTING_HAPTIC_INCREMENT,
  SETTING_DPAD_ANGLE_SIN,
  SETTING_DPAD_ANGLE_COS,
  SETTING_MOMENTUM_VERTICAL_DIVISOR,
  SETTING_MOMENTUM_MAXIMUM_VELOCITY,
  SETTING_TRACKPAD_Z_ON,

  // 20
  SETTING_TRACKPAD_Z_OFF,
  SETTING_SENSITIVITY_SCALE_AMOUNT,
  SETTING_LEFT_TRACKPAD_SECONDARY_MODE,
  SETTING_RIGHT_TRACKPAD_SECONDARY_MODE,
  SETTING_SMOOTH_ABSOLUTE_MOUSE,
  SETTING_STEAMBUTTON_POWEROFF_TIME,
  SETTING_UNUSED_1,
  SETTING_TRACKPAD_OUTER_RADIUS,
  SETTING_TRACKPAD_Z_ON_LEFT,
  SETTING_TRACKPAD_Z_OFF_LEFT,

  // 30
  SETTING_TRACKPAD_OUTER_SPIN_VEL,
  SETTING_TRACKPAD_OUTER_SPIN_RADIUS,
  SETTING_TRACKPAD_OUTER_SPIN_HORIZONTAL_ONLY,
  SETTING_TRACKPAD_RELATIVE_MODE_DEADZONE,
  SETTING_TRACKPAD_RELATIVE_MODE_MAX_VEL,
  SETTING_TRACKPAD_RELATIVE_MODE_INVERT_Y,
  SETTING_TRACKPAD_DOUBLE_TAP_BEEP_ENABLED,
  SETTING_TRACKPAD_DOUBLE_TAP_BEEP_PERIOD,
  SETTING_TRACKPAD_DOUBLE_TAP_BEEP_COUNT,
  SETTING_TRACKPAD_OUTER_RADIUS_RELEASE_ON_TRANSITION,

  // 40
  SETTING_RADIAL_MODE_ANGLE,
  SETTING_HAPTIC_INTENSITY_MOUSE_MODE,
  SETTING_LEFT_DPAD_REQUIRES_CLICK,
  SETTING_RIGHT_DPAD_REQUIRES_CLICK,
  SETTING_LED_BASELINE_BRIGHTNESS,
  SETTING_LED_USER_BRIGHTNESS,
  SETTING_ENABLE_RAW_JOYSTICK,
  SETTING_ENABLE_FAST_SCAN,
  SETTING_IMU_MODE,
  SETTING_WIRELESS_PACKET_VERSION,

  // 50
  SETTING_SLEEP_INACTIVITY_TIMEOUT,
  SETTING_TRACKPAD_NOISE_THRESHOLD,
  SETTING_LEFT_TRACKPAD_CLICK_PRESSURE,
  SETTING_RIGHT_TRACKPAD_CLICK_PRESSURE,
  SETTING_LEFT_BUMPER_CLICK_PRESSURE,
  SETTING_RIGHT_BUMPER_CLICK_PRESSURE,
  SETTING_LEFT_GRIP_CLICK_PRESSURE,
  SETTING_RIGHT_GRIP_CLICK_PRESSURE,
  SETTING_LEFT_GRIP2_CLICK_PRESSURE,
  SETTING_RIGHT_GRIP2_CLICK_PRESSURE,

  // 60
  SETTING_PRESSURE_MODE,
  SETTING_CONTROLLER_TEST_MODE,
  SETTING_TRIGGER_MODE,
  SETTING_TRACKPAD_Z_THRESHOLD,
  SETTING_FRAME_RATE,
  SETTING_TRACKPAD_FILT_CTRL,
  SETTING_TRACKPAD_CLIP,
  SETTING_DEBUG_OUTPUT_SELECT,
  SETTING_TRIGGER_THRESHOLD_PERCENT,
  SETTING_TRACKPAD_FREQUENCY_HOPPING,

  // 70
  SETTING_HAPTICS_ENABLED,
  SETTING_STEAM_WATCHDOG_ENABLE,
  SETTING_TIMP_TOUCH_THRESHOLD_ON,
  SETTING_TIMP_TOUCH_THRESHOLD_OFF,
  SETTING_FREQ_HOPPING,
  SETTING_TEST_CONTROL,
  SETTING_HAPTIC_MASTER_GAIN_DB,
  SETTING_THUMB_TOUCH_THRESH,
  SETTING_DEVICE_POWER_STATUS,
  SETTING_HAPTIC_INTENSITY,

  // 80
  SETTING_STABILIZER_ENABLED,
  SETTING_TIMP_MODE_MTE,
  SETTING_COUNT,

  // This is a special setting value use for callbacks and should not be set/get explicitly.
  SETTING_ALL = 0xFF
} ControllerSettingID;

typedef enum {
  DEVICE_KEYBOARD,
  DEVICE_MOUSE,
  DEVICE_GAMEPAD,
  DEVICE_MODE_ADJUST,
  DEVICE_COUNT
} DeviceType;

enum MouseButtons {
  MOUSE_BTN_LEFT = 1,
  MOUSE_BTN_RIGHT,
  MOUSE_BTN_MIDDLE,
  MOUSE_BTN_BACK,
  MOUSE_BTN_FORWARD,
  MOUSE_SCROLL_UP,
  MOUSE_SCROLL_DOWN,
  MOUSE_BTN_COUNT
};

typedef enum {
  TRACKPAD_ABSOLUTE_MOUSE,
  TRACKPAD_RELATIVE_MOUSE,
  TRACKPAD_DPAD_FOUR_WAY_DISCRETE,
  TRACKPAD_DPAD_FOUR_WAY_OVERLAP,
  TRACKPAD_DPAD_EIGHT_WAY,
  TRACKPAD_RADIAL_MODE,
  TRACKPAD_ABSOLUTE_DPAD,
  TRACKPAD_NONE,
  TRACKPAD_GESTURE_KEYBOARD,
  TRACKPAD_NUM_MODES
} TrackpadMode;

typedef enum {
  STEAM_DECK_LBUTTON_R2              = 0x00000001,
  STEAM_DECK_LBUTTON_L2              = 0x00000002,
  STEAM_DECK_LBUTTON_R               = 0x00000004,
  STEAM_DECK_LBUTTON_L               = 0x00000008,
  STEAM_DECK_LBUTTON_Y               = 0x00000010,
  STEAM_DECK_LBUTTON_B               = 0x00000020,
  STEAM_DECK_LBUTTON_X               = 0x00000040,
  STEAM_DECK_LBUTTON_A               = 0x00000080,
  STEAM_DECK_LBUTTON_DPAD_UP         = 0x00000100,
  STEAM_DECK_LBUTTON_DPAD_RIGHT      = 0x00000200,
  STEAM_DECK_LBUTTON_DPAD_LEFT       = 0x00000400,
  STEAM_DECK_LBUTTON_DPAD_DOWN       = 0x00000800,
  STEAM_DECK_LBUTTON_VIEW            = 0x00001000,
  STEAM_DECK_LBUTTON_STEAM           = 0x00002000,
  STEAM_DECK_LBUTTON_MENU            = 0x00004000,
  STEAM_DECK_LBUTTON_L5              = 0x00008000,
  STEAM_DECK_LBUTTON_R5              = 0x00010000,
  STEAM_DECK_LBUTTON_LEFT_PAD        = 0x00020000,
  STEAM_DECK_LBUTTON_RIGHT_PAD       = 0x00040000,
  STEAM_DECK_LBUTTON_LEFT_PAD_TOUCH  = 0x00080000,
  STEAM_DECK_LBUTTON_RIGHT_PAD_TOUCH = 0x00100000,
  STEAM_DECK_LBUTTON_L3              = 0x00400000,
  STEAM_DECK_LBUTTON_R3              = 0x04000000,

  STEAM_DECK_HBUTTON_L4              = 0x00000200,
  STEAM_DECK_HBUTTON_R4              = 0x00000400,
  STEAM_DECK_HBUTTON_L3_TOUCH        = 0x00004000,
  STEAM_DECK_HBUTTON_R3_TOUCH        = 0x00008000,
  STEAM_DECK_HBUTTON_QAM             = 0x00040000,
} SteamDeckButton;

typedef struct {
  unsigned char type;
  unsigned char length;
} FeatureReportHeader;

typedef struct {
  FeatureReportHeader header;
  guint8 rumble_type;
  guint16 intensity;
  guint16 left_motor_speed;
  guint16 right_motor_speed;
  gint8 left_gain;
  gint8 right_gain;
} RumbleReport;

typedef struct {
  unsigned char setting;
  unsigned short value;
} ControllerSetting;

typedef struct {
  unsigned char button_mask[8];
  unsigned char device;
  unsigned char value;
} DigitalMapping;

typedef struct {
  FeatureReportHeader header;
  ControllerSetting settings[15];
} SetSettingsReport;

G_STATIC_ASSERT (sizeof (SetSettingsReport) <= HID_FEATURE_REPORT_BYTES);

typedef struct {
  FeatureReportHeader header;
  DigitalMapping mappings[6];
} SetDigitalMappingReport;

G_STATIC_ASSERT (sizeof (DigitalMapping) == 10);

typedef struct {
  unsigned short version;
  unsigned char type;
  unsigned char length;
} InputReportHeader;

typedef struct {
  // If packet num matches that on your prior call, then the controller
  // state hasn't been changed since your last call and there is no need to
  // process it
  guint32 packet_num;

  // Button bitmask and trigger data.
  guint32 buttons_l;
  guint32 buttons_h;

  // Left pad coordinates
  short left_pad_x;
  short left_pad_y;

  // Right pad coordinates
  short right_pad_x;
  short right_pad_y;

  // Accelerometer values
  short accel_x;
  short accel_y;
  short accel_z;

  // Gyroscope values
  short gyro_x;
  short gyro_y;
  short gyro_z;

  // Gyro quaternions
  short gyro_quat_w;
  short gyro_quat_x;
  short gyro_quat_y;
  short gyro_quat_z;

  // Uncalibrated trigger values
  unsigned short trigger_raw_l;
  unsigned short trigger_raw_r;

  // Left stick values
  short left_stick_x;
  short left_stick_y;

  // Right stick values
  short right_stick_x;
  short right_stick_y;

  // Touchpad pressures
  unsigned short pressure_pad_left;
  unsigned short pressure_pad_right;
} SteamDeckState;

typedef struct {
  InputReportHeader header;
  SteamDeckState deck_state;
} InputReport;

struct _ManetteSteamDeckDriver {
  GObject parent_instance;

  hid_device *hid;

  guint rumble_timeout;

  guint32 last_packet;
  guint32 last_buttons_l;
  guint32 last_buttons_h;
  short last_left_stick_x;
  short last_left_stick_y;
  short last_right_stick_x;
  short last_right_stick_y;
  short last_trigger_l;
  short last_trigger_r;

  int lizard_watchdog_counter;
};

static void manette_steam_deck_hid_driver_init (ManetteHidDriverInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ManetteSteamDeckDriver, manette_steam_deck_driver, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (MANETTE_TYPE_HID_DRIVER, manette_steam_deck_hid_driver_init))

static gboolean
send_simple_feature_report (ManetteSteamDeckDriver *self,
                            FeatureReportMessageID  id)
{
  guint8 buffer[sizeof (FeatureReportHeader) + 1] = { 0 };
  FeatureReportHeader *header = (FeatureReportHeader *) (buffer + 1);

  header->type = id;

  return hid_send_feature_report (self->hid, buffer, sizeof (buffer)) > 0;
}

static inline void
set_button_mask (SteamDeckButton  buttons_l,
                 SteamDeckButton  buttons_h,
                 unsigned char   *dest)
{
  memcpy (dest, &buttons_l, sizeof (guint32));
  memcpy (dest + 4, &buttons_h, sizeof (guint32));
}

static gboolean
send_set_digital_mappings (ManetteSteamDeckDriver *self,
                           SteamDeckButton         first_buttons_l,
                           SteamDeckButton         first_buttons_h,
                           DeviceType              first_device,
                           int                     first_value,
                           ...)
{
  guint8 buffer[HID_FEATURE_REPORT_BYTES + 1] = { 0 };
  SetDigitalMappingReport *report = (SetDigitalMappingReport *) (buffer + 1);
  int n_mappings = 1;
  va_list args;

  report->header.type = ID_SET_DIGITAL_MAPPINGS;

  set_button_mask (first_buttons_l, first_buttons_h, report->mappings[0].button_mask);
  report->mappings[0].device = first_device;
  report->mappings[0].value = first_value;

  va_start (args, first_value);

  while (TRUE) {
    SteamDeckButton buttons_l = va_arg (args, SteamDeckButton);
    SteamDeckButton buttons_h;
    DeviceType device;
    unsigned char value;

    if ((int) buttons_l < 0)
      break;

    g_assert (n_mappings < G_N_ELEMENTS (report->mappings));

    buttons_h = va_arg (args, SteamDeckButton);
    device = va_arg (args, DeviceType);
    value = va_arg (args, int);

    set_button_mask (buttons_l, buttons_h, report->mappings[n_mappings].button_mask);
    report->mappings[n_mappings].device = device;
    report->mappings[n_mappings].value = value;

    n_mappings++;
  }

  report->header.length = n_mappings * sizeof (DigitalMapping);

  va_end (args);

  return hid_send_feature_report (self->hid, buffer, sizeof (buffer)) > 0;
}

static gboolean
send_set_settings (ManetteSteamDeckDriver *self,
                   ControllerSettingID     first_setting,
                   int                     first_value,
                   ...)
{
  guint8 buffer[HID_FEATURE_REPORT_BYTES + 1] = { 0 };
  SetSettingsReport *report = (SetSettingsReport *) (buffer + 1);
  int n_settings = 1;
  va_list args;

  report->header.type = ID_SET_SETTINGS_VALUES;

  report->settings[0].setting = first_setting;
  report->settings[0].value = first_value;

  va_start (args, first_value);

  while (TRUE) {
    ControllerSettingID setting = va_arg (args, ControllerSettingID);
    short value;

    if ((int) setting < 0)
      break;

    g_assert (n_settings < G_N_ELEMENTS (report->settings));

    value = va_arg (args, int);

    report->settings[n_settings].setting = setting;
    report->settings[n_settings].value = value;

    n_settings++;
  }

  report->header.length = n_settings * sizeof(ControllerSetting);

  va_end (args);

  int r = hid_send_feature_report (self->hid, buffer, sizeof (buffer));
  if (r < 0)
    return FALSE;

  // There may be a lingering report read back after changing settings.
  // Discard it.
  hid_get_feature_report (self->hid, buffer, sizeof (buffer));

  return TRUE;
}

static gboolean
disable_lizard_mode (ManetteSteamDeckDriver *self)
{
  if (!send_simple_feature_report (self, ID_CLEAR_DIGITAL_MAPPINGS))
    return FALSE;

  if (!send_set_digital_mappings (self,
                                  STEAM_DECK_LBUTTON_LEFT_PAD, 0,
                                  DEVICE_MOUSE, MOUSE_BTN_RIGHT,
                                  STEAM_DECK_LBUTTON_RIGHT_PAD, 0,
                                  DEVICE_MOUSE, MOUSE_BTN_LEFT,
                          -1)) {
    return FALSE;
  }

  if (!send_simple_feature_report (self, ID_LOAD_DEFAULT_SETTINGS))
    return FALSE;

  if (!send_set_settings (self,
                          SETTING_LEFT_TRACKPAD_MODE, TRACKPAD_NONE,
                          -1)) {
    return FALSE;
  }

  return TRUE;
}

static void
send_absolute_event (ManetteSteamDeckDriver *self,
                     ManetteAxis             axis,
                     short                   value,
                     gint64                  time,
                     gboolean                inverted)
{
  double axis_value;

  if (inverted)
    axis_value = (double) value / -32767.0;
  else
    axis_value = (double) value / 32767.0;

  manette_hid_driver_emit_axis_event (MANETTE_HID_DRIVER (self),
                                      time, axis, axis_value);
}

static void
handle_button_l (ManetteSteamDeckDriver *self,
                 SteamDeckState         *state,
                 gint64                  time,
                 int                     deck_button,
                 ManetteButton           button)
{
  gboolean old_pressed = (self->last_buttons_l & deck_button) > 0;
  gboolean new_pressed = (state->buttons_l & deck_button) > 0;

  if (old_pressed == new_pressed)
    return;

  manette_hid_driver_emit_button_event (MANETTE_HID_DRIVER (self),
                                        time, button, new_pressed);
}

static void
handle_button_h (ManetteSteamDeckDriver *self,
                 SteamDeckState         *state,
                 gint64                  time,
                 int                     deck_button,
                 ManetteButton           button)
{
  gboolean old_pressed = (self->last_buttons_h & deck_button) > 0;
  gboolean new_pressed = (state->buttons_h & deck_button) > 0;

  if (old_pressed == new_pressed)
    return;

  manette_hid_driver_emit_button_event (MANETTE_HID_DRIVER (self),
                                        time, button, new_pressed);
}

static inline short
normalize_stick (short value)
{
  if (value > -STICK_FLAT && value < STICK_FLAT)
    return 0;

  return value;
}

static void
handle_state (ManetteSteamDeckDriver *self,
              SteamDeckState         *state,
              gint64                  time)
{
  short normalized_left_stick_x, normalized_left_stick_y;
  short normalized_right_stick_x, normalized_right_stick_y;

  if (state->packet_num == self->last_packet)
    return;

  self->last_packet = state->packet_num;

  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_R,          MANETTE_BUTTON_RIGHT_SHOULDER);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_L,          MANETTE_BUTTON_LEFT_SHOULDER);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_Y,          MANETTE_BUTTON_NORTH);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_B,          MANETTE_BUTTON_EAST);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_X,          MANETTE_BUTTON_WEST);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_A,          MANETTE_BUTTON_SOUTH);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_DPAD_UP,    MANETTE_BUTTON_DPAD_UP);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_DPAD_RIGHT, MANETTE_BUTTON_DPAD_RIGHT);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_DPAD_LEFT,  MANETTE_BUTTON_DPAD_LEFT);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_DPAD_DOWN,  MANETTE_BUTTON_DPAD_DOWN);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_VIEW,       MANETTE_BUTTON_SELECT);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_STEAM,      MANETTE_BUTTON_MODE);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_MENU,       MANETTE_BUTTON_START);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_L5,         MANETTE_BUTTON_LEFT_PADDLE2);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_R5,         MANETTE_BUTTON_RIGHT_PADDLE2);
  // touchpads
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_L3,         MANETTE_BUTTON_LEFT_STICK);
  handle_button_l (self, state, time, STEAM_DECK_LBUTTON_R3,         MANETTE_BUTTON_RIGHT_STICK);
  handle_button_h (self, state, time, STEAM_DECK_HBUTTON_L4,         MANETTE_BUTTON_LEFT_PADDLE1);
  handle_button_h (self, state, time, STEAM_DECK_HBUTTON_R4,         MANETTE_BUTTON_RIGHT_PADDLE1);
  handle_button_h (self, state, time, STEAM_DECK_HBUTTON_QAM,        MANETTE_BUTTON_MISC1);

  normalized_left_stick_x = normalize_stick (state->left_stick_x);
  normalized_left_stick_y = normalize_stick (state->left_stick_y);
  normalized_right_stick_x = normalize_stick (state->right_stick_x);
  normalized_right_stick_y = normalize_stick (state->right_stick_y);

  if (self->last_left_stick_x != normalized_left_stick_x)
    send_absolute_event (self, MANETTE_AXIS_LEFT_X, normalized_left_stick_x, time, FALSE);

  if (self->last_left_stick_y != normalized_left_stick_y)
    send_absolute_event (self, MANETTE_AXIS_LEFT_Y, normalized_left_stick_y, time, TRUE);

  if (self->last_right_stick_x != normalized_right_stick_x)
    send_absolute_event (self, MANETTE_AXIS_RIGHT_X, normalized_right_stick_x, time, FALSE);

  if (self->last_right_stick_y != normalized_right_stick_y)
    send_absolute_event (self, MANETTE_AXIS_RIGHT_Y, normalized_right_stick_y, time, TRUE);

  if (self->last_trigger_l != state->trigger_raw_l)
    send_absolute_event (self, MANETTE_AXIS_LEFT_TRIGGER, state->trigger_raw_l, time, FALSE);

  if (self->last_trigger_r != state->trigger_raw_r)
    send_absolute_event (self, MANETTE_AXIS_RIGHT_TRIGGER, state->trigger_raw_r, time, FALSE);

  self->last_buttons_l = state->buttons_l;
  self->last_buttons_h = state->buttons_h;
  self->last_left_stick_x = normalized_left_stick_x;
  self->last_left_stick_y = normalized_left_stick_y;
  self->last_right_stick_x = normalized_right_stick_x;
  self->last_right_stick_y = normalized_right_stick_y;
  self->last_trigger_l = state->trigger_raw_l;
  self->last_trigger_r = state->trigger_raw_r;
}

static void
manette_steam_deck_driver_finalize (GObject *object)
{
  ManetteSteamDeckDriver *self = MANETTE_STEAM_DECK_DRIVER (object);

  g_clear_handle_id (&self->rumble_timeout, g_source_remove);

  G_OBJECT_CLASS (manette_steam_deck_driver_parent_class)->finalize (object);
}

static void
manette_steam_deck_driver_class_init (ManetteSteamDeckDriverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = manette_steam_deck_driver_finalize;
}

static void
manette_steam_deck_driver_init (ManetteSteamDeckDriver *self)
{
}

static gboolean
manette_steam_deck_driver_initialize (ManetteHidDriver *driver)
{
  ManetteSteamDeckDriver *self = MANETTE_STEAM_DECK_DRIVER (driver);
  guint8 data[HID_FEATURE_REPORT_BYTES];

  int size = hid_read_timeout (self->hid, data, sizeof (data), 16);
  if (size == 0)
    return FALSE;

  if (!disable_lizard_mode (self))
    return FALSE;

  return TRUE;
}

static char *
manette_steam_deck_driver_get_name (ManetteHidDriver *driver)
{
  return g_strdup ("Steam Deck");
}

static gboolean
manette_steam_deck_driver_has_button (ManetteHidDriver *driver,
                                      ManetteButton     button)
{
  switch (button) {
  case MANETTE_BUTTON_DPAD_UP:
  case MANETTE_BUTTON_DPAD_DOWN:
  case MANETTE_BUTTON_DPAD_LEFT:
  case MANETTE_BUTTON_DPAD_RIGHT:
  case MANETTE_BUTTON_NORTH:
  case MANETTE_BUTTON_SOUTH:
  case MANETTE_BUTTON_WEST:
  case MANETTE_BUTTON_EAST:
  case MANETTE_BUTTON_SELECT:
  case MANETTE_BUTTON_START:
  case MANETTE_BUTTON_MODE:
  case MANETTE_BUTTON_LEFT_SHOULDER:
  case MANETTE_BUTTON_RIGHT_SHOULDER:
  case MANETTE_BUTTON_LEFT_STICK:
  case MANETTE_BUTTON_RIGHT_STICK:
  case MANETTE_BUTTON_LEFT_PADDLE1:
  case MANETTE_BUTTON_LEFT_PADDLE2:
  case MANETTE_BUTTON_RIGHT_PADDLE1:
  case MANETTE_BUTTON_RIGHT_PADDLE2:
  case MANETTE_BUTTON_MISC1:
    return TRUE;

  default:
    return FALSE;
  }
}

static gboolean
manette_steam_deck_driver_has_axis (ManetteHidDriver *driver,
                                    ManetteAxis       axis)
{
  switch (axis) {
  case MANETTE_AXIS_LEFT_X:
  case MANETTE_AXIS_LEFT_Y:
  case MANETTE_AXIS_RIGHT_X:
  case MANETTE_AXIS_RIGHT_Y:
  case MANETTE_AXIS_LEFT_TRIGGER:
  case MANETTE_AXIS_RIGHT_TRIGGER:
    return TRUE;

  default:
    return FALSE;
  }
}

static guint
manette_steam_deck_driver_get_poll_rate (ManetteHidDriver *driver)
{
  return 4;
}

static void
manette_steam_deck_driver_poll (ManetteHidDriver *driver,
                                gint64            time)
{
  ManetteSteamDeckDriver *self = MANETTE_STEAM_DECK_DRIVER (driver);
  guint8 buffer[64];
  InputReport *report = (InputReport *) buffer;
  int read;

  if (self->lizard_watchdog_counter++ > 200) {
    self->lizard_watchdog_counter = 0;
    disable_lizard_mode (self);
  }

  memset (buffer, 0, sizeof (buffer));

  while (TRUE) {
    read = hid_read (self->hid, buffer, sizeof (buffer));

    if (read < 0) {
      g_debug ("Failed to get input report: %ls", hid_error (self->hid));
      return;
    }

    if (read == 0)
      break;

    if (report->header.version != INPUT_REPORT_VERSION ||
        report->header.type != ID_CONTROLLER_DECK_STATE ||
        report->header.length != 64) {
      continue;
    }

    handle_state (self, &report->deck_state, time);
  }
}

static gboolean
manette_steam_deck_driver_has_rumble (ManetteHidDriver *driver)
{
  return TRUE;
}

static gboolean
send_rumble (ManetteSteamDeckDriver *self,
             guint16                 left_speed,
             guint16                 right_speed)
{
  guint8 buffer[sizeof (RumbleReport) + 1] = { 0 };
  RumbleReport *report = (RumbleReport *) (buffer + 1);

  report->header.type = ID_TRIGGER_RUMBLE_CMD;
  report->rumble_type = 0;
  report->intensity = HAPTIC_INTENSITY_SYSTEM;
  report->left_motor_speed = left_speed;
  report->right_motor_speed = right_speed;
  report->left_gain = 2;
  report->right_gain = 0;

  int r = hid_send_feature_report (self->hid, buffer, sizeof (buffer));
  if (r < 0) {
    g_warning ("Failed to rumble: %ls", hid_error (self->hid));

    return FALSE;
  }

  return TRUE;
}

static void
stop_rumble_cb (ManetteSteamDeckDriver *self)
{
  self->rumble_timeout = 0;

  send_rumble (self, 0, 0);
}

static gboolean
manette_steam_deck_driver_rumble (ManetteHidDriver *driver,
                                  guint16           strong_magnitude,
                                  guint16           weak_magnitude,
                                  guint16           milliseconds)
{
  ManetteSteamDeckDriver *self = MANETTE_STEAM_DECK_DRIVER (driver);

  if (!send_rumble (self, strong_magnitude, weak_magnitude)) {
    g_warning ("Failed to rumble.");

    return FALSE;
  }

  g_clear_handle_id (&self->rumble_timeout, g_source_remove);

  self->rumble_timeout = g_timeout_add_once (milliseconds,
                                             (GSourceOnceFunc) stop_rumble_cb,
                                             self);

  return TRUE;
}

static void
manette_steam_deck_hid_driver_init (ManetteHidDriverInterface *iface)
{
  iface->initialize = manette_steam_deck_driver_initialize;
  iface->get_name = manette_steam_deck_driver_get_name;
  iface->has_button = manette_steam_deck_driver_has_button;
  iface->has_axis = manette_steam_deck_driver_has_axis;
  iface->get_poll_rate = manette_steam_deck_driver_get_poll_rate;
  iface->poll = manette_steam_deck_driver_poll;
  iface->has_rumble = manette_steam_deck_driver_has_rumble;
  iface->rumble = manette_steam_deck_driver_rumble;
}

ManetteHidDriver *
manette_steam_deck_driver_new (hid_device *hid)
{
  ManetteSteamDeckDriver *self = g_object_new (MANETTE_TYPE_STEAM_DECK_DRIVER, NULL);

  self->hid = hid;

  return MANETTE_HID_DRIVER (self);
}
