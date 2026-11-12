/*
 * StickRemote -- M5StickS3 firmware: the stick on the rifle as the WindMaster 360 RANGE's remote.
 *
 * 3.5 was 3.4 (../StickHoldRecorder34) with the recording taken out: no IMU, no PSRAM, no flash
 * recordings, no downloads. What is left is the one-button menu and the LIVE stream the phone's
 * 360 RANGE (its M5Stick trigger) listens to. ../StickHoldRecorder34 stays as the last recorder.
 * 3.6 adds CIRCLE SIZE: the red circle the phone draws round the cross, grown or shrunk to the edge
 * of what the scope shows of the screen. 3.7 drops CENTRE RETICLE (the phone's centre key still does it)
 * for WIND -- the range's MAX WIND up or down, down to none -- and MAGNIFICATION. 3.8 adds a TRIGGER
 * BUTTON wired to the Grove port (below). 3.9 drops WIND again, puts START/EXIT at the top of the menu,
 * reorders it, makes a click of B leave SHOOT and MAGNIFICATION, steps MAGNIFICATION along a list, and
 * dims the screen and then switches the stick off when nobody touches it (AUTO POWER OFF, below).
 * 4.0 adds ZERO GYRO: with the rifle rested, the phone measures its gyro's own error while both screens
 * count down (ZERO GYRO, below).
 *
 * ZERO GYRO (4.0)
 *   A on the menu's ZERO GYRO tells the phone (LIVE flag bit 4) to measure its gyro's error. The phone
 *   counts down, 5 s with the rifle still, starting again if it moves, and sends each second as "cal <n>";
 *   this screen shows it in big type. "cal 0" is done: READY, back to the menu on START/EXIT. "cal -1" is
 *   cancelled. A on the countdown, or the phone's own button, cancels. The phone's setup page has the
 *   same button, and its count shows here too.
 *
 * AUTO POWER OFF (3.9)
 *   Every press of A, B or the trigger, the phone connecting and the range opening count as use. After
 *   DIM_AFTER_US without any the screen dims (a press still does what it always does, and lights it
 *   again). After OFF_AFTER_US -- OFF_AFTER_LIVE_US while the phone's range is listening -- the last
 *   OFF_WARN_US count down on screen; a press of A or B then only keeps the stick on, and at zero it
 *   switches itself off for real (the power chip cuts the battery). The side button switches it on again.
 *
 * TRIGGER BUTTON (3.8)
 *   A push button between the Grove port's GND and G9 or G10 (either: both are watched). Pins counted
 *   from the GND end: 0 GND, 1 5V -- NEVER wire the button to it -- 2 G9, 3 G10. With a Grove cable:
 *   black = GND, yellow = G9. A 4-leg button switches between DIAGONALLY opposite legs.
 *   It fires whenever the phone's 360 RANGE is listening, on every screen (not only SHOOT): a trigger
 *   is only ever for shooting. The shot goes out as A's does. Debounced over TRIGGER_DEBOUNCE_US.
 *
 * SETUP (Arduino IDE 2)
 *   - File > Preferences > Additional boards manager URLs:
 *       https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
 *   - Boards Manager: install "M5Stack" (3.2.5 or newer).
 *   - Library Manager: install "M5Unified" (0.2.12 or newer; accept M5GFX when it asks) and
 *     "NimBLE-Arduino" (2.x).
 *   - Tools > Board > M5Stack > M5StickS3, and leave that board's defaults alone.
 *   - No partitions.csv any more: the board's default layout is fine, nothing is kept in flash.
 *   - Upload. If no port shows up, hold the side button for about 2 s until the green LED blinks
 *     (download mode), upload, then press it once to restart.
 *
 * USE: ONE MENU
 *   MENU:       B = next item              A = open it              hold B = turn the screen 180 degrees
 *     START/EXIT      A, straight from the menu: the phone STARTs the range from its setup page (the
 *                     LABORATORY's, or STANDING / KNEELING's Practice) with the settings on it, or EXITs a
 *                     range being shot. Then SHOOT opens. The stick cannot know which: the phone decides
 *     ZERO GYRO       A, with the rifle resting still: the phone measures its gyro's error, both screens
 *                     count down 5 s, then READY (4.0). A again cancels
 *     SHOOT           A fires on the 360 RANGE; a click of B is back to the menu (3.9; before, it fired)
 *     RETICLE ON/OFF  A shows or hides the red cross and its circle (straight from the menu)
 *     MOVE RETICLE    B = next direction (UP RIGHT DOWN LEFT), A = move the red cross 1 px
 *     TURN RETICLE    B = LEFT / RIGHT, A = turn the red cross 0.1 degree
 *     CIRCLE SIZE     B = BIGGER / SMALLER, A = the red circle round the cross 1 px more or less
 *     MAGNIFICATION   A = the next magnification on the phone's list, the lowest after the highest;
 *                     a click of B is back to the menu (3.9; before, B = UP / DOWN and A = 1x)
 *   Inside any of them, hold B = back to the menu. Holding A repeats in MOVE / TURN / CIRCLE: every
 *   60 ms and faster after 1.5 s. MAGNIFICATION takes one step a press.
 *   - Every item needs the phone's 360 RANGE with its M5Stick trigger. Until it connects they are
 *     grey: B still steps through them (3.4 skipped grey items, so with no phone B seemed dead), and
 *     A on one says what is missing. The phone opening the range opens the menu on START/EXIT (before
 *     3.9, SHOOT): what it is opened on is its setup page, waiting to be started.
 *
 * THE LINK
 *   Commands are text lines: from USB, or written to the Bluetooth CMD characteristic one per write.
 *   Replies go back the way the command came: to USB, or as notifications on DATA. STATUS is a short
 *   "key=value ... ;" line, notified on every change and every five seconds.
 *     s | status        a status line
 *     live on | off     the stick as the 360 RANGE's remote: see LIVE below
 *     cal <n>           ZERO GYRO's countdown from the phone: n seconds left, 0 = READY, -1 = cancelled (4.0)
 *   Anything else, the recorder's commands included, answers "#error unknown command".
 *
 * LIVE: THE STICK AS THE 360 RANGE'S REMOTE (3.4's 8 bytes, two more since 3.7)
 *   "live on" makes the menu's items work for as long as the link stays up. Each change goes out at
 *   once as a 10-byte LIVE (49f40005) notification, and once a second without one, so the phone
 *   always shows the screen the stick is on:
 *     0  sequence
 *     1  flags: bit 0 fired, bit 1 show/hide the cross, bit 3 START/EXIT (3.9), bit 4 ZERO GYRO (4.0) --
 *        each since the last
 *        packet (bit 2, centre, is 3.6's and no longer sent)
 *     2  screen: 0 SHOOT, 1 MOVE, 2 TURN, 3 anything else, 4 CIRCLE, 6 MAGNIFICATION (3 stays "else" for
 *        older apps, which read 6 as "else" too; 5 was 3.7's WIND and is no longer sent)
 *     3  direction: MOVE 0-3 = UP RIGHT DOWN LEFT, TURN 0 = LEFT, 1 = RIGHT, CIRCLE 0 = SMALLER, 1 = BIGGER,
 *        MAGNIFICATION 2 = NEXT since 3.9 (3.7 and 3.8: 0 = DOWN, 1 = UP)
 *     4  int8 the cross's move since the last packet, screen px, x right
 *     5  int8 the same, y down
 *     6  int8 the cross's turn since the last packet, tenths of a degree, + = clockwise
 *     7  int8 the circle's radius change since the last packet, screen px, + = bigger (0 before 3.6)
 *     8  always 0 since 3.9 (3.7's MAX WIND change; kept so byte 9 stays where the app reads it)
 *     9  int8 the magnification change since the last packet: with NEXT, how many steps along the
 *        phone's list; before 3.9, whole x, + = more (3.7)
 *   An app older than 3.7 reads the first 8 and ignores the rest. "live off" or the link dropping ends it.
 *
 * Bluetooth callbacks only queue the command text for loop() to run.
 */

#include <M5Unified.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <esp_timer.h>

static constexpr const char* FIRMWARE = "StickRemote 4.0";

// ------------------------------------------------------------------ Bluetooth

// A random base, 49f4xxxx-4f6b-44d8-ac39-091b92128517, numbered from 0001. The app has the same four.
static constexpr const char* BLE_SERVICE = "49f40001-4f6b-44d8-ac39-091b92128517";
static constexpr const char* BLE_CMD     = "49f40002-4f6b-44d8-ac39-091b92128517";  // write: one command
static constexpr const char* BLE_DATA    = "49f40003-4f6b-44d8-ac39-091b92128517";  // notify: the replies
static constexpr const char* BLE_STATUS  = "49f40004-4f6b-44d8-ac39-091b92128517";  // read + notify
static constexpr const char* BLE_LIVE    = "49f40005-4f6b-44d8-ac39-091b92128517";  // notify: the aim stream

// ------------------------------------------------------------------ sizes and buttons

// The largest DATA notification, whatever MTU the phone agrees: 180 is iOS's usual 185 less the ATT
// header, and every phone takes it.
static constexpr size_t      BLE_CHUNK_MAX   = 180;
static constexpr uint8_t     BRIGHTNESS      = 48;
static constexpr uint32_t    B_HOLD_MS       = 900;              // hold B this long: back to the menu, or turn the screen
static constexpr int64_t     INFO_SHOW_US    = 4000000;          // a note replaces the menu's hint this long
static constexpr int64_t     LIVE_HEARTBEAT_US = 1000000;        // a LIVE packet at least this often
static constexpr size_t      LIVE_BYTES      = 10;
static constexpr uint8_t     LIVE_FIRE       = 0x01;             // fired since the last packet
static constexpr uint8_t     LIVE_RETICLE    = 0x02;             // show / hide the red cross
static constexpr uint8_t     LIVE_START_EXIT = 0x08;             // START the range from its setup, or EXIT it (3.9)
static constexpr uint8_t     LIVE_ZERO       = 0x10;             // ZERO GYRO: start, or cancel, the phone's count (4.0)
static constexpr int64_t     CAL_MOVED_SHOW_US = 1500000;        // "MOVED" stays up this long after the count starts again
static constexpr int64_t     NUDGE_HOLD_US   = 400000;           // MOVE / TURN / ...: A held this long starts repeating...
static constexpr int64_t     NUDGE_REPEAT_US = 60000;            // ...a step this often...
static constexpr int64_t     NUDGE_FAST_US   = 1500000;          // ...and fast steps after this long
// AUTO POWER OFF (3.9): unused this long, the screen dims, and then the stick switches itself off.
static constexpr int64_t     DIM_AFTER_US      = 60LL * 1000000;        // 1 min
static constexpr int64_t     OFF_AFTER_US      = 5LL * 60 * 1000000;    // 5 min with no range listening...
static constexpr int64_t     OFF_AFTER_LIVE_US = 15LL * 60 * 1000000;   // ...15 while the phone's range is
static constexpr int64_t     OFF_WARN_US       = 30LL * 1000000;        // the last 30 s count down on screen
static constexpr uint8_t     DIM_BRIGHTNESS    = 6;
// The trigger button on the Grove port (3.8): to GND, read with the ESP32's pull-ups.
static constexpr uint8_t     TRIGGER_PINS[2] = { 9, 10 };
static constexpr int64_t     TRIGGER_DEBOUNCE_US = 20000;        // a contact must read the same this long to count
static constexpr int16_t     NUDGE_FAST_PX   = 5;                // MOVE's fast step
static constexpr int16_t     TURN_FAST_TENTHS = 5;               // TURN's: 0.5 degree, the phone's turn keys' own
static constexpr int16_t     CIRCLE_FAST_PX  = 5;                // CIRCLE's fast step, radius px
static const char* const     DIR_NAMES[4]    = { "UP", "RIGHT", "DOWN", "LEFT" };
static const char* const     TURN_NAMES[2]   = { "LEFT", "RIGHT" };
static const char* const     CIRCLE_NAMES[2] = { "SMALLER", "BIGGER" };
// MAGNIFICATION's direction byte since 3.9: each step is the next on the phone's list, not 1x up or down.
static constexpr uint8_t     LIVE_MAG_NEXT   = 2;

// The menu, top to bottom. Every item drives the phone's 360 RANGE, so needs it live.
enum MenuItem : uint8_t { ITEM_START, ITEM_ZERO, ITEM_SHOOT, ITEM_RETICLE, ITEM_MOVE, ITEM_TURN, ITEM_CIRCLE, ITEM_MAG, MENU_ITEMS };
static const char* const     ITEM_NAMES[MENU_ITEMS] = {
  "START/EXIT", "ZERO GYRO", "SHOOT", "RETICLE ON/OFF", "MOVE RETICLE", "TURN RETICLE", "CIRCLE SIZE", "MAGNIFICATION" };

// ------------------------------------------------------------------ state

// What the screen shows and the buttons do.
enum class Screen { MENU, SHOOT, MOVE, TURN, CIRCLE, MAG, ZERO };

struct CommandMsg {
  char text[64];
};

static bool     spriteOk = false;
static uint8_t  rotation = 3;       // landscape; 1 is the same turned 180 degrees
static Screen   screen = Screen::MENU;
static uint8_t  menuItem = ITEM_START;  // the menu's highlighted item
static char     infoText[48] = "";
static int64_t  infoAtUs = 0;       // when infoText was written: the hint line shows it for INFO_SHOW_US
static char     stickId[16] = "";   // the ESP32's factory MAC, as hex
static Preferences prefs;

static int      liveBattery = -1;

// The remote ("live on"), and what the next LIVE packet has to carry.
static bool     liveOn = false;
static uint8_t  liveDir = 0;            // MOVE's direction, an index into DIR_NAMES
static uint8_t  turnDir = 1;            // TURN's, an index into TURN_NAMES
static uint8_t  liveFlags = 0;
static int16_t  liveDx = 0, liveDy = 0; // the cross's move not yet sent, screen px
static int16_t  liveTurn = 0;           // its turn not yet sent, tenths of a degree, + = clockwise
static uint8_t  circleDir = 1;          // CIRCLE's, an index into CIRCLE_NAMES
static int16_t  liveCircle = 0;         // the circle's radius change not yet sent, px, + = bigger
static int16_t  liveMag = 0;            // the magnification change not yet sent, x, + = more
static int      calLeft = -1;           // ZERO GYRO's seconds left as the phone last sent them; -1 = none yet
static int64_t  calMovedUs = 0;         // when the count last started again (the rifle moved)
static bool     liveDirty = false;      // screen or direction changed since the last packet
static uint8_t  liveSeq = 0;
static uint32_t liveFires = 0;
static int64_t  lastLiveUs = 0;
static bool     nudgeArmed = false;     // A went down on this stepping screen, so holding it repeats
static int64_t  nudgeDownUs = 0, nudgeNextUs = 0;

static int64_t  lastDrawUs = 0, lastBatteryUs = 0, lastStatusUs = 0;
static bool     statusDirty = true;

// AUTO POWER OFF: when the stick was last used, whether the screen is dimmed, and whether the buttons
// are being ignored until both are up (the press that called off the countdown).
static int64_t  usedAtUs = 0;
static bool     dimmed = false;
static bool     swallowButtons = false;
static bool     wasConnected = false;

static char     cmdBuf[64];
static size_t   cmdLen = 0;

// Written by the NimBLE host task, read by loop().
static volatile bool     bleConnected = false;
static volatile uint16_t bleConnHandle = 0;
static volatile uint16_t bleMtu = 23;
static QueueHandle_t     bleCommands = nullptr;
static NimBLECharacteristic* dataChr = nullptr;
static NimBLECharacteristic* statusChr = nullptr;
static NimBLECharacteristic* liveChr = nullptr;

static M5Canvas canvas(&M5.Display);

// ------------------------------------------------------------------ output: USB or Bluetooth

/**
 * A reply going out over Bluetooth: bytes collect into one notification's worth and go as a DATA
 * notification. notify() says false when NimBLE is out of buffers, and that is the flow control: wait
 * a moment and offer it again, rather than lose it.
 */
class BleReply : public Print {
 public:
  bool failed = false;

  size_t write(uint8_t b) override { return write(&b, 1); }

  size_t write(const uint8_t* p, size_t n) override {
    size_t done = 0;
    while (done < n) {
      const size_t room = chunk_ - len_;
      const size_t k = (n - done) < room ? (n - done) : room;
      memcpy(buf_ + len_, p + done, k);
      len_ += k;
      done += k;
      if (len_ >= chunk_) send();
    }
    return n;
  }

  void begin() {
    len_ = 0;
    failed = false;
    size_t c = (size_t)bleMtu > 3 ? (size_t)bleMtu - 3 : 20;
    if (c < 20) c = 20;
    if (c > BLE_CHUNK_MAX) c = BLE_CHUNK_MAX;
    chunk_ = c;
  }

  void finish() {
    if (len_ > 0) send();
  }

 private:
  void send() {
    if (!failed && bleConnected && dataChr != nullptr) {
      const uint32_t since = millis();
      while (!dataChr->notify(buf_, len_, bleConnHandle)) {
        if (!bleConnected || millis() - since > 3000) {
          failed = true;
          break;
        }
        delay(2);
      }
    } else {
      failed = true;
    }
    len_ = 0;
  }

  uint8_t buf_[BLE_CHUNK_MAX];
  size_t  len_ = 0;
  size_t  chunk_ = 20;
};

static BleReply bleReply;

// ------------------------------------------------------------------ status

/** The phone's 360 RANGE is listening, so the menu's items work. */
static bool remoteOn() {
  return liveOn && bleConnected;
}

/** The STATUS line: short enough for one notification on any MTU a phone negotiates, ';' last. */
static void statusLine(char* out, size_t size) {
  snprintf(out, size, "st=ready bat=%d live=%d id=%s;", liveBattery, liveOn ? 1 : 0, stickId);
}

static void publishStatus() {
  if (statusChr == nullptr) return;
  char line[64];
  statusLine(line, sizeof(line));
  statusChr->setValue((const uint8_t*)line, strlen(line));
  if (bleConnected) statusChr->notify(bleConnHandle);
  lastStatusUs = esp_timer_get_time();
  statusDirty = false;
}

static void printStatus(Print& out) {
  if (&out == &Serial) {
    Serial.printf("#status fw=\"%s\" ble=%d mtu=%u live=%d battery=%d\n",
                  FIRMWARE, bleConnected ? 1 : 0, (unsigned)bleMtu, liveOn ? 1 : 0, liveBattery);
  } else {
    char line[64];
    statusLine(line, sizeof(line));
    out.printf("#status %s\n#end\n", line);
  }
}

// ------------------------------------------------------------------ the aim stream

static void openScreen(Screen s) {
  screen = s;
  nudgeArmed = false;
  liveDirty = true;
}

/** Back to the menu, on [item]. */
static void openMenu(uint8_t item) {
  menuItem = item;
  openScreen(Screen::MENU);
}

static void markUsed();

static void startLive() {
  liveOn = true;
  liveFlags = 0;
  liveDx = liveDy = 0;
  liveTurn = 0;
  liveCircle = 0;
  liveMag = 0;
  liveFires = 0;
  openMenu(ITEM_START);              // the range opens on its setup page, waiting to be started
  statusDirty = true;
  markUsed();
}

static void fire() {
  liveFlags |= LIVE_FIRE;
  ++liveFires;
}

/** MOVE: [px] more in the chosen direction, for the next packet. */
static void nudge(int16_t px) {
  switch (liveDir) {
    case 0: liveDy -= px; break;
    case 1: liveDx += px; break;
    case 2: liveDy += px; break;
    case 3: liveDx -= px; break;
  }
}

/** TURN: [tenths] of a degree more in the chosen direction, for the next packet. */
static void turn(int16_t tenths) {
  liveTurn += turnDir == 1 ? tenths : -tenths;
}

/** CIRCLE: the radius [px] bigger or smaller, as chosen, for the next packet. */
static void resize(int16_t px) {
  liveCircle += circleDir == 1 ? px : -px;
}

/**
 * The stepping screens: a step on the press, then while A stays down, a step every [repeatUs] from
 * NUDGE_HOLD_US on, [fast] a step after NUDGE_FAST_US. Only a press made on this screen repeats: the A
 * that opened it from the menu is still down when it appears.
 */
static void stepWhileHeld(int64_t now, void (*step)(int16_t), int16_t fast, int64_t repeatUs) {
  if (M5.BtnA.wasPressed()) {
    step(1);
    nudgeArmed = true;
    nudgeDownUs = now;
    nudgeNextUs = now + NUDGE_HOLD_US;
  } else if (!M5.BtnA.isPressed()) {
    nudgeArmed = false;
  } else if (nudgeArmed && now >= nudgeNextUs) {
    step(now - nudgeDownUs >= NUDGE_FAST_US ? fast : 1);
    nudgeNextUs = now + repeatUs;
  }
}

/** The LIVE packet's screen byte: the phone shows the stepping screens' state, and nothing for the rest. */
static uint8_t liveScreenCode() {
  switch (screen) {
    case Screen::SHOOT:  return 0;
    case Screen::MOVE:   return 1;
    case Screen::TURN:   return 2;
    case Screen::CIRCLE: return 4;
    case Screen::MAG:    return 6;
    default:             return 3;
  }
}

/** The LIVE packet's direction byte, for the screen on show. */
static uint8_t liveDirCode() {
  switch (screen) {
    case Screen::TURN:   return turnDir;
    case Screen::CIRCLE: return circleDir;
    case Screen::MAG:    return LIVE_MAG_NEXT;
    default:             return liveDir;
  }
}

static int8_t takeInt8(int16_t& v) {
  const int16_t k = v > 127 ? 127 : (v < -127 ? -127 : v);
  v -= k;
  return (int8_t)k;
}

/**
 * A LIVE packet as soon as there is something to say, else once a second. One NimBLE has no buffer
 * for is offered again on the next loop, with anything that has happened since.
 */
static void sendLive(int64_t nowUs) {
  if (!liveOn || !bleConnected || liveChr == nullptr) return;
  const bool news = liveFlags != 0 || liveDx != 0 || liveDy != 0 || liveTurn != 0 || liveCircle != 0 ||
                    liveMag != 0 || liveDirty;
  if (!news && nowUs - lastLiveUs < LIVE_HEARTBEAT_US) return;
  int16_t dx = liveDx, dy = liveDy, dt = liveTurn, dc = liveCircle, dm = liveMag;
  uint8_t p[LIVE_BYTES];
  p[0] = liveSeq;
  p[1] = liveFlags;
  p[2] = liveScreenCode();
  p[3] = liveDirCode();
  p[4] = (uint8_t)takeInt8(dx);
  p[5] = (uint8_t)takeInt8(dy);
  p[6] = (uint8_t)takeInt8(dt);
  p[7] = (uint8_t)takeInt8(dc);
  p[8] = 0;                          // 3.7's MAX WIND
  p[9] = (uint8_t)takeInt8(dm);
  if (!liveChr->notify(p, LIVE_BYTES, bleConnHandle)) return;
  lastLiveUs = nowUs;
  ++liveSeq;
  liveFlags = 0;
  liveDx = dx;
  liveDy = dy;
  liveTurn = dt;
  liveCircle = dc;
  liveMag = dm;
  liveDirty = false;
}

// ------------------------------------------------------------------ commands

static void note(const char* text);

/**
 * ZERO GYRO's count from the phone (4.0): [n] seconds left -- the countdown screen opens if it is not on
 * show, the phone's own button having started it -- 0 = READY, back to the menu on START/EXIT, or -1 =
 * cancelled. A count that goes UP means the rifle moved and it started again: MOVED shows for a moment.
 */
static void showCalibration(int n) {
  markUsed();
  if (n > 0) {
    if (calLeft >= 0 && n > calLeft) calMovedUs = esp_timer_get_time();
    calLeft = n;
    if (screen != Screen::ZERO && remoteOn()) {
      menuItem = ITEM_ZERO;
      openScreen(Screen::ZERO);
    }
  } else if (n == 0) {
    calLeft = -1;
    if (screen == Screen::ZERO) openMenu(ITEM_START);
    note("gyro zeroed: READY");
  } else {
    calLeft = -1;
    if (screen == Screen::ZERO) openMenu(ITEM_ZERO);
    note("zero gyro cancelled");
  }
}

static void handleCommand(const char* cmd, Print& out) {
  if (strcmp(cmd, "s") == 0 || strcmp(cmd, "status") == 0) {
    printStatus(out);
  } else if (strcmp(cmd, "live on") == 0) {
    startLive();
    out.print("#ok live on\n#end\n");
  } else if (strncmp(cmd, "cal ", 4) == 0) {
    showCalibration(atoi(cmd + 4));
    out.print("#ok cal\n#end\n");
  } else if (strcmp(cmd, "live off") == 0) {
    liveOn = false;                  // loop() leaves any range screen
    statusDirty = true;
    out.print("#ok live off\n#end\n");
  } else {
    out.printf("#error unknown command \"%s\"\n#end\n", cmd);
  }
}

static void handleSerial() {
  while (Serial.available() > 0) {
    const int c = Serial.read();
    if (c < 0) break;
    if (c == '\n' || c == '\r') {
      cmdBuf[cmdLen] = 0;
      if (cmdLen > 0) handleCommand(cmdBuf, Serial);
      cmdLen = 0;
    } else if (cmdLen < sizeof(cmdBuf) - 1) {
      cmdBuf[cmdLen++] = (char)c;
    }
  }
}

static void handleBle() {
  CommandMsg msg;
  while (bleCommands != nullptr && xQueueReceive(bleCommands, &msg, 0) == pdTRUE) {
    bleReply.begin();
    handleCommand(msg.text, bleReply);
    bleReply.finish();
  }
}

// ------------------------------------------------------------------ Bluetooth callbacks (NimBLE task)

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& info) override {
    bleConnHandle = info.getConnHandle();
    bleMtu = info.getMTU();
    bleConnected = true;
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& info, int reason) override {
    bleConnected = false;
    bleMtu = 23;
    NimBLEDevice::startAdvertising();
  }

  void onMTUChange(uint16_t mtu, NimBLEConnInfo& info) override {
    bleMtu = mtu;
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info) override {
    const NimBLEAttValue value = chr->getValue();
    CommandMsg msg;
    size_t n = value.length();
    if (n > sizeof(msg.text) - 1) n = sizeof(msg.text) - 1;
    memcpy(msg.text, value.data(), n);
    while (n > 0 && (msg.text[n - 1] == '\n' || msg.text[n - 1] == '\r' || msg.text[n - 1] == ' ')) --n;
    msg.text[n] = 0;
    if (n > 0 && bleCommands != nullptr) xQueueSend(bleCommands, &msg, 0);
  }
};

static ServerCallbacks serverCallbacks;
static CommandCallbacks commandCallbacks;

static void setupBle() {
  char name[24];
  snprintf(name, sizeof(name), "WM-Stick-%s", stickId + 8);    // the MAC's last four hex digits
  bleCommands = xQueueCreate(8, sizeof(CommandMsg));
  NimBLEDevice::init(name);
  NimBLEDevice::setMTU(517);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  NimBLEService* service = server->createService(BLE_SERVICE);
  NimBLECharacteristic* cmd = service->createCharacteristic(BLE_CMD, NIMBLE_PROPERTY::WRITE);
  cmd->setCallbacks(&commandCallbacks);
  dataChr = service->createCharacteristic(BLE_DATA, NIMBLE_PROPERTY::NOTIFY);
  statusChr = service->createCharacteristic(BLE_STATUS, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  liveChr = service->createCharacteristic(BLE_LIVE, NIMBLE_PROPERTY::NOTIFY);
  server->start();                           // NimBLE 2 starts the services with the server

  // The 128-bit service goes in the advertisement, where the app's scan filter looks for it; the
  // name does not fit beside it in 31 bytes, so it rides in the scan response.
  NimBLEAdvertisementData advert;
  advert.setFlags(0x02 | 0x04);              // general discoverable, no classic Bluetooth
  advert.addServiceUUID(NimBLEUUID(BLE_SERVICE));
  NimBLEAdvertisementData scanReply;
  scanReply.setName(name);
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setAdvertisementData(advert);
  advertising->setScanResponseData(scanReply);
  advertising->start();
  publishStatus();
}

// ------------------------------------------------------------------ screen

/** The top-right corner on every screen, size 1: the phone, the battery. */
static void drawCorner(lgfx::LovyanGFX& g) {
  char bat[8];
  const char* phone = remoteOn() ? "RANGE  " : (bleConnected ? "PHONE  " : "");
  snprintf(bat, sizeof(bat), "%d%%", liveBattery);
  g.setTextSize(1);
  g.setCursor(g.width() - 4 - 6 * (int)(strlen(phone) + strlen(bat)), 4);
  g.setTextColor(TFT_CYAN, TFT_BLACK);
  g.print(phone);
  g.setTextColor(TFT_WHITE, TFT_BLACK);
  g.print(bat);
}

/** A screen's name, top left, size 2 -- short, so it clears the corner. */
static void drawTitle(lgfx::LovyanGFX& g, const char* title, uint16_t color) {
  g.setTextSize(2);
  g.setTextColor(color, TFT_BLACK);
  g.setCursor(4, 4);
  g.print(title);
}

/** [names] in a row, size 1, the chosen one bright: what B steps through. */
static void drawChoices(lgfx::LovyanGFX& g, int y, const char* const* names, int count, int chosen) {
  g.setTextSize(1);
  g.setCursor(4, y);
  for (int k = 0; k < count; ++k) {
    g.setTextColor(k == chosen ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
    g.print(names[k]);
    g.print("   ");
  }
}

static void drawMenu(lgfx::LovyanGFX& g) {
  g.setTextSize(1);
  g.setTextColor(TFT_DARKGREY, TFT_BLACK);
  g.setCursor(4, 4);
  g.print("MENU");
  g.setTextSize(2);
  const uint16_t color = remoteOn() ? TFT_WHITE : TFT_DARKGREY;
  // Eight rows 14 px apart (4.0; seven 15 apart before): the size-2 font draws 14 px of its 16, so they
  // just meet, and the last ends above the bottom line, where the hint goes.
  for (uint8_t k = 0; k < MENU_ITEMS; ++k) {
    const int y = 12 + 14 * k;
    const bool chosen = k == menuItem;
    const uint16_t back = chosen ? TFT_NAVY : TFT_BLACK;
    if (chosen) g.fillRect(0, y, g.width(), 14, back);
    g.setTextColor(color, back);
    g.setCursor(4, y);
    g.print(chosen ? "> " : "  ");
    g.print(ITEM_NAMES[k]);
  }
}

/** AUTO POWER OFF's last seconds, in place of whatever screen was on show. */
static void drawCountdown(lgfx::LovyanGFX& g, int seconds) {
  char line[24];
  drawTitle(g, "POWER OFF", TFT_ORANGE);
  g.setTextSize(4);
  g.setTextColor(TFT_WHITE, TFT_BLACK);
  g.setCursor(4, 40);
  snprintf(line, sizeof(line), "IN %d s", seconds);
  g.print(line);
  g.setTextSize(1);
  g.setTextColor(TFT_DARKGREY, TFT_BLACK);
  g.setCursor(4, 115);
  g.print("A or B: stay on");
  g.setCursor(4, 125);
  g.print("off: the side button turns it on");
}

/** Seconds of AUTO POWER OFF's countdown left at [now], or -1 when it has not begun. */
static int countdownSeconds(int64_t now);

static void drawTo(lgfx::LovyanGFX& g) {
  char line[24];
  g.fillScreen(TFT_BLACK);
  g.setTextWrap(false);
  const int countdown = countdownSeconds(esp_timer_get_time());
  if (countdown >= 0) {
    drawCountdown(g, countdown);
    drawCorner(g);
    return;
  }
  const char* hint1 = "";
  const char* hint2 = "hold B: menu";

  switch (screen) {
    case Screen::MENU:
      drawMenu(g);
      hint1 = "";                      // the rows reach down to the first hint line
      hint2 = "B: next  A: open  hold B: flip screen";
      break;
    case Screen::SHOOT:
      drawTitle(g, "SHOOT", TFT_CYAN);
      g.setTextSize(3);
      g.setTextColor(TFT_WHITE, TFT_BLACK);
      g.setCursor(4, 44);
      snprintf(line, sizeof(line), "%lu SHOTS", (unsigned long)liveFires);
      g.print(line);
      hint1 = "A or the trigger: fire";
      hint2 = "B: menu";
      break;
    case Screen::MOVE:
      drawTitle(g, "MOVE", TFT_YELLOW);
      g.setTextSize(4);
      g.setTextColor(TFT_WHITE, TFT_BLACK);
      g.setCursor(4, 36);
      g.print(DIR_NAMES[liveDir]);
      drawChoices(g, 84, DIR_NAMES, 4, liveDir);
      hint1 = "A: move 1 px     hold A: repeat";
      hint2 = "B: direction     hold B: menu";
      break;
    case Screen::TURN:
      drawTitle(g, "TURN", TFT_YELLOW);
      g.setTextSize(4);
      g.setTextColor(TFT_WHITE, TFT_BLACK);
      g.setCursor(4, 36);
      g.print(TURN_NAMES[turnDir]);
      drawChoices(g, 84, TURN_NAMES, 2, turnDir);
      hint1 = "A: turn 0.1 deg  hold A: repeat";
      hint2 = "B: left/right    hold B: menu";
      break;
    case Screen::CIRCLE:
      drawTitle(g, "CIRCLE", TFT_YELLOW);
      g.setTextSize(4);
      g.setTextColor(TFT_WHITE, TFT_BLACK);
      g.setCursor(4, 36);
      g.print(CIRCLE_NAMES[circleDir]);
      drawChoices(g, 84, CIRCLE_NAMES, 2, circleDir);
      hint1 = "A: resize 1 px   hold A: repeat";
      hint2 = "B: bigger/smaller  hold B: menu";
      break;
    case Screen::ZERO: {
      const bool moved = esp_timer_get_time() - calMovedUs < CAL_MOVED_SHOW_US;
      drawTitle(g, "ZERO GYRO", TFT_YELLOW);
      g.setTextSize(5);
      g.setTextColor(TFT_WHITE, TFT_BLACK);
      g.setCursor(4, 34);
      if (calLeft >= 0) {
        snprintf(line, sizeof(line), "%d s", calLeft);
        g.print(line);
      } else {
        g.print("...");
      }
      g.setTextSize(2);
      g.setTextColor(moved ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
      g.setCursor(4, 84);
      g.print(moved ? "MOVED: AGAIN" : "KEEP STILL");
      hint1 = "calibrating gyro error";
      hint2 = "A: cancel   B: menu";
      break;
    }
    case Screen::MAG:
      drawTitle(g, "MAG", TFT_YELLOW);
      g.setTextSize(4);
      g.setTextColor(TFT_WHITE, TFT_BLACK);
      g.setCursor(4, 36);
      g.print("NEXT");
      g.setTextSize(1);
      g.setTextColor(TFT_DARKGREY, TFT_BLACK);
      g.setCursor(4, 84);
      g.print("the phone shows which; max -> min");
      hint1 = "A: next magnification";
      hint2 = "B: menu";
      break;
  }
  drawCorner(g);

  // The bottom two lines: what the buttons do, unless a note is showing for a few seconds -- on the
  // first line, or on the menu, whose rows take the first line, on the second.
  const bool noting = infoText[0] != 0 && esp_timer_get_time() - infoAtUs < INFO_SHOW_US;
  g.setTextSize(1);
  g.setCursor(4, 115);
  if (noting && screen != Screen::MENU) {
    g.setTextColor(TFT_CYAN, TFT_BLACK);
    g.print(infoText);
  } else {
    g.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g.print(hint1);
  }
  g.setCursor(4, 125);
  if (noting && screen == Screen::MENU) {
    g.setTextColor(TFT_CYAN, TFT_BLACK);
    g.print(infoText);
  } else {
    g.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g.print(hint2);
  }
}

static void drawScreen() {
  if (spriteOk) {
    drawTo(canvas);
    canvas.pushSprite(0, 0);
  } else {
    drawTo(M5.Display);
  }
}

// ------------------------------------------------------------------ the buttons

/** A short note in the hint line's place for a few seconds. */
static void note(const char* text) {
  snprintf(infoText, sizeof(infoText), "%s", text);
  infoAtUs = esp_timer_get_time();
}

/** The menu's A: open the item's screen, or do it there and then; without the range, say so. */
static void selectItem(uint8_t item) {
  if (!remoteOn()) {
    note(bleConnected ? "open 360 RANGE, M5Stick trigger" : "connect the phone's 360 RANGE");
    return;
  }
  switch (item) {
    case ITEM_START:
      // A toggle as well: the phone starts a range from its setup page and exits one being shot. SHOOT
      // next, since a start is followed by shooting; hold B comes back here, on START/EXIT, to exit.
      liveFlags |= LIVE_START_EXIT;
      openScreen(Screen::SHOOT);
      note("range started / exited");
      break;
    case ITEM_ZERO:
      // The phone measures and counts; this screen shows its count as "cal <n>" arrives.
      liveFlags |= LIVE_ZERO;
      calLeft = -1;
      openScreen(Screen::ZERO);
      break;
    case ITEM_SHOOT:   openScreen(Screen::SHOOT); break;
    case ITEM_MOVE:    openScreen(Screen::MOVE); break;
    case ITEM_TURN:    openScreen(Screen::TURN); break;
    case ITEM_CIRCLE:  openScreen(Screen::CIRCLE); break;
    case ITEM_MAG:     openScreen(Screen::MAG); break;
    case ITEM_RETICLE:
      // A toggle: the stick cannot know which way the phone has it.
      liveFlags |= LIVE_RETICLE;
      note("red cross + circle shown / hidden");
      break;
  }
}

/** One loop's buttons, for the screen on show; true when the screen needs drawing again. */
static bool handleButtons(int64_t now) {
  bool redraw = false;
  // The range's screens need the range: the link dropping, or "live off", sends the stick back to
  // the menu, on the item it was on.
  if (liveOn && !bleConnected) {
    liveOn = false;
    statusDirty = true;
  }
  if (screen != Screen::MENU && !remoteOn()) {
    openMenu(menuItem);
    redraw = true;
  }

  if (M5.BtnB.wasHold()) {
    if (screen == Screen::MENU) {    // both rotations are landscape, so the sprite still fits
      rotation = rotation == 3 ? 1 : 3;
      M5.Display.setRotation(rotation);
      prefs.putUChar("rot", rotation);
    } else {
      openMenu(menuItem);            // the item the screen was opened from
    }
    return true;
  }

  switch (screen) {
    case Screen::MENU:
      // Every item, grey or not: with no phone, B still visibly moves.
      if (M5.BtnB.wasClicked()) {
        menuItem = (uint8_t)((menuItem + 1) % MENU_ITEMS);
        redraw = true;
      } else if (M5.BtnA.wasPressed()) {
        selectItem(menuItem);
        redraw = true;
      }
      break;
    case Screen::SHOOT:
      // A fires on the press itself; a click of B is the way back, no hold needed (3.9; it fired before).
      if (M5.BtnA.wasPressed()) {
        fire();
        redraw = true;
      } else if (M5.BtnB.wasClicked()) {
        openMenu(menuItem);
        redraw = true;
      }
      break;
    case Screen::MOVE:
      if (M5.BtnB.wasClicked()) {
        liveDir = (uint8_t)((liveDir + 1) % 4);
        liveDirty = true;
        redraw = true;
      }
      stepWhileHeld(now, nudge, NUDGE_FAST_PX, NUDGE_REPEAT_US);
      break;
    case Screen::TURN:
      if (M5.BtnB.wasClicked()) {
        turnDir = (uint8_t)(turnDir ^ 1);
        liveDirty = true;
        redraw = true;
      }
      stepWhileHeld(now, turn, TURN_FAST_TENTHS, NUDGE_REPEAT_US);
      break;
    case Screen::CIRCLE:
      if (M5.BtnB.wasClicked()) {
        circleDir = (uint8_t)(circleDir ^ 1);
        liveDirty = true;
        redraw = true;
      }
      stepWhileHeld(now, resize, CIRCLE_FAST_PX, NUDGE_REPEAT_US);
      break;
    case Screen::ZERO:
      // A asks the phone again, which cancels a running count (or starts one after a cancel).
      if (M5.BtnA.wasPressed()) {
        liveFlags |= LIVE_ZERO;
        calLeft = -1;
        redraw = true;
      } else if (M5.BtnB.wasClicked()) {
        openMenu(menuItem);
        redraw = true;
      }
      break;
    case Screen::MAG:
      // A: the next magnification on the phone's list, round to the first after the last; one a press.
      // A click of B is the way back, as in SHOOT.
      if (M5.BtnA.wasPressed()) {
        ++liveMag;
        redraw = true;
      } else if (M5.BtnB.wasClicked()) {
        openMenu(menuItem);
        redraw = true;
      }
      break;
  }
  return redraw;
}

// ------------------------------------------------------------------ AUTO POWER OFF (3.9)

/** How long the stick may go unused before it switches off: longer while the phone's range listens. */
static int64_t offAfterUs() {
  return remoteOn() ? OFF_AFTER_LIVE_US : OFF_AFTER_US;
}

/** The stick in use now: the clock starts again and the screen lights up. */
static void markUsed() {
  usedAtUs = esp_timer_get_time();
  if (dimmed) {
    dimmed = false;
    M5.Display.setBrightness(BRIGHTNESS);
  }
}

static int countdownSeconds(int64_t now) {
  const int64_t left = usedAtUs + offAfterUs() - now;
  if (left > OFF_WARN_US) return -1;
  return left <= 0 ? 0 : (int)((left + 999999) / 1000000);
}

/**
 * Once a loop, before the buttons: A or B down is use, and so is the phone connecting. In the countdown
 * a press only keeps the stick on, and the buttons are then ignored until both are up, so its release
 * does nothing either. Dims the screen, lights it again for the countdown, and at its end switches the
 * stick off. False when the buttons are not to be read this loop.
 */
static bool pollPower(int64_t now, bool& redraw) {
  const bool connected = bleConnected;
  if (connected != wasConnected) {
    wasConnected = connected;
    if (connected) markUsed();
  }
  const bool down = M5.BtnA.isPressed() || M5.BtnB.isPressed();
  if (swallowButtons) {
    if (!down) swallowButtons = false;   // from the next loop on: this one may hold the release's click
    return false;
  }
  const int countdown = countdownSeconds(now);
  if (down) {
    markUsed();
    if (countdown >= 0) {                // the countdown called off, and nothing else
      swallowButtons = true;
      redraw = true;
      return false;
    }
    return true;
  }
  if (countdown == 0) {
    M5.Power.powerOff();                 // does not come back when the power chip cuts the battery...
    note("on USB power: stays on");      // ...but USB keeps it running
    markUsed();
    redraw = true;
  } else if (countdown > 0) {
    if (dimmed) {                        // the countdown has to be read
      dimmed = false;
      M5.Display.setBrightness(BRIGHTNESS);
    }
  } else if (!dimmed && now - usedAtUs >= DIM_AFTER_US) {
    dimmed = true;
    M5.Display.setBrightness(DIM_BRIGHTNESS);
  }
  return true;
}

// ------------------------------------------------------------------ the trigger button (3.8)

static bool    triggerRaw = false;      // what the pins read last loop: pressed when either is at GND
static bool    triggerDown = false;     // the debounced state
static int64_t triggerSinceUs = 0;      // when triggerRaw last changed

/**
 * The Grove trigger: a shot on the debounced press, whatever screen is on show, for as long as the
 * range listens; without it, a note says so. True when the screen needs drawing again.
 */
static bool pollTrigger(int64_t now) {
  bool raw = false;
  for (uint8_t pin : TRIGGER_PINS) raw = raw || digitalRead(pin) == LOW;
  if (raw != triggerRaw) {
    triggerRaw = raw;
    triggerSinceUs = now;
    return false;
  }
  if (raw == triggerDown || now - triggerSinceUs < TRIGGER_DEBOUNCE_US) return false;
  triggerDown = raw;
  if (!triggerDown) return false;      // the release says nothing
  markUsed();                          // and a shot, even in the countdown: a trigger is never ignored
  if (remoteOn()) {
    fire();
  } else {
    note(bleConnected ? "open 360 RANGE, M5Stick trigger" : "connect the phone's 360 RANGE");
  }
  return true;
}

// ------------------------------------------------------------------ Arduino

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;   // USB CDC ignores the rate
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  M5.begin(cfg);

  const uint64_t mac = ESP.getEfuseMac();
  snprintf(stickId, sizeof(stickId), "%04x%08lx", (unsigned)((mac >> 32) & 0xFFFF), (unsigned long)(mac & 0xFFFFFFFFu));

  prefs.begin("stickhold", false);   // 3.4's namespace: the screen keeps its rotation across the update
  rotation = prefs.getUChar("rot", 3);
  if (rotation != 1 && rotation != 3) rotation = 3;

  M5.Display.setRotation(rotation);
  M5.Display.setBrightness(BRIGHTNESS);
  M5.Display.fillScreen(TFT_BLACK);
  M5.BtnB.setHoldThresh(B_HOLD_MS);  // A is only ever pressed, never held for a meaning of its own
  for (uint8_t pin : TRIGGER_PINS) pinMode(pin, INPUT_PULLUP);   // after M5.begin(), so nothing resets them

  canvas.setColorDepth(16);
  spriteOk = canvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;

  liveBattery = M5.Power.getBatteryLevel();
  usedAtUs = esp_timer_get_time();   // switched on is used
  setupBle();
  drawScreen();
  printStatus(Serial);
}

void loop() {
  M5.update();
  const int64_t now = esp_timer_get_time();

  if (now - lastBatteryUs >= 10000000) {
    lastBatteryUs = now;
    const int battery = M5.Power.getBatteryLevel();
    if (battery != liveBattery) {
      liveBattery = battery;
      statusDirty = true;
    }
  }

  bool redraw = false;
  if (pollPower(now, redraw) && handleButtons(now)) redraw = true;
  if (pollTrigger(now)) redraw = true;

  handleSerial();
  handleBle();
  sendLive(esp_timer_get_time());

  if (statusDirty || now - lastStatusUs >= 5000000) publishStatus();

  if (redraw || now - lastDrawUs >= 250000) {
    lastDrawUs = now;
    drawScreen();
  }
  delay(1);
}
