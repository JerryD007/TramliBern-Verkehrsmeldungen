#pragma once
#include <Arduino_GFX_Library.h>
#include "IntroAssets.h"
#include "IntroCrests.h"

// Shared renderer. A compact indexed motion band is composed before one SPI transfer.
class BootIntro {
public:
  static constexpr uint32_t durationMs = 3000;
  static constexpr uint32_t frameMs = 33; // Approximately 30 fps; retain deadline phase.
  static constexpr int tramWidth = 260, tramHeight = 104, tramY = 158;
  static constexpr int leftEdge = 18, rightEdge = 462;
  static int position(uint32_t elapsed) {
    if (elapsed > durationMs) elapsed = durationMs;
    return leftEdge - tramWidth + int((rightEdge-leftEdge+tramWidth) * elapsed / durationMs);
  }
  void cancel() { running=false; }
  bool active() const { return running; }
  uint8_t city() const { return selectedCity; }
  void begin(Arduino_GFX *display, uint32_t now, int requestedCity = 0) {
    selectedCity = requestedCity >= 0 && requestedCity < IntroAssets::cityCount ? requestedCity : 0;
    started = now; lastFrame = now; previousX = leftEdge - tramWidth;
    running = true;
    paint(display, 0, 0, 480, 320, previousX);
  }
  // True once on completion. Caller must invalidate the board's drawing cache.
  bool tick(Arduino_GFX *display, uint32_t now) {
    if (!running) return false;
    const uint32_t elapsed = now - started;
    if (elapsed >= durationMs) { running = false; return true; }
    if (uint32_t(now-lastFrame) < frameMs) return false;
    lastFrame = now - (elapsed % frameMs);
    const int x = position(elapsed);
    int left = previousX < x ? previousX : x;
    int right = (previousX > x ? previousX : x) + tramWidth;
    if (left < leftEdge) left = leftEdge;
    if (right > rightEdge) right = rightEdge;
    if (right > left) paintMotion(display, left, right-left, x);
    previousX = x;
    return false;
  }
private:
  bool running = false;
  uint8_t selectedCity = 0;
  uint32_t started = 0, lastFrame = 0;
  int previousX = leftEdge - tramWidth;
  // Shared storage: startup strips and the motion band are never used together.
  union {
    uint16_t strip[480*4];
    uint8_t frame[(rightEdge-leftEdge)*tramHeight];
  };
  uint16_t motionPalette[48];
  void paintMotion(Arduino_GFX *display, int x, int w, int tramX) {
    for (int i=0;i<16;++i) {
      motionPalette[i]=pgm_read_word(IntroAssets::backgroundPalettes[selectedCity]+i);
      motionPalette[16+i]=pgm_read_word(IntroCrests::palettes[selectedCity]+i);
      motionPalette[32+i]=pgm_read_word(IntroAssets::vehiclePalettes[selectedCity]+i);
    }
    const uint8_t *background=IntroAssets::backgrounds[selectedCity];
    const uint8_t *vehicle=IntroAssets::vehicles[IntroAssets::vehicleTypes[selectedCity]];
    for (int dy=0;dy<tramHeight;++dy) {
      const int py=tramY+dy;
      const int backgroundRow=(py*IntroAssets::bgHeight/320)*IntroAssets::bgWidth;
      for (int dx=0;dx<w;++dx) {
        const int px=x+dx;
        uint8_t color=index(background,backgroundRow+px*IntroAssets::bgWidth/480);
        const int cx=px-29,cy=py-24;
        if(cx>=0 && cx<IntroCrests::width && cy>=0 && cy<IntroCrests::height) {
          const uint8_t c=index(IntroCrests::pixels[selectedCity],cy*IntroCrests::width+cx);
          if(c!=15) color=16+c;
        }
        const int sx=px-tramX;
        if(sx>=0 && sx<tramWidth) {
          const uint8_t t=index(vehicle,dy*tramWidth+sx);
          if(t!=15) color=32+t;
        }
        frame[dy*w+dx]=color;
      }
    }
    // ST7796/Arduino_TFT sends this in one address window and SPI transaction.
    display->drawIndexedBitmap(x,tramY,frame,motionPalette,w,tramHeight);
  }
  static uint8_t index(const uint8_t *data, uint32_t offset) {
    const uint8_t v = pgm_read_byte(data + offset/2);
    return offset & 1 ? v & 15 : v >> 4;
  }
  void paint(Arduino_GFX *display, int x, int y, int w, int h, int tramX) {
    const uint8_t *background = IntroAssets::backgrounds[selectedCity];
    const uint8_t *vehicle = IntroAssets::vehicles[IntroAssets::vehicleTypes[selectedCity]];
    for (int row=y; row<y+h; row+=4) {
      const int rows = y+h-row < 4 ? y+h-row : 4;
      for (int dy=0; dy<rows; ++dy) for (int dx=0; dx<w; ++dx) {
        const int px=x+dx, py=row+dy;
        const uint32_t off = (py*IntroAssets::bgHeight/320)*IntroAssets::bgWidth + px*IntroAssets::bgWidth/480;
        uint16_t color = pgm_read_word(IntroAssets::backgroundPalettes[selectedCity] + index(background,off));
        // Separate full-resolution crests: never enlarge the 24x26 menu icons.
        const int cx=px-29, cy=py-24;
        if (cx>=0 && cx<IntroCrests::width && cy>=0 && cy<IntroCrests::height) {
          const uint8_t c=index(IntroCrests::pixels[selectedCity],cy*IntroCrests::width+cx);
          if (c!=15) color=pgm_read_word(IntroCrests::palettes[selectedCity]+c);
        }
        const int sx=px-tramX, sy=py-tramY;
        if (px>=leftEdge && px<rightEdge && sx>=0 && sx<tramWidth && sy>=0 && sy<tramHeight) {
          const uint8_t t=index(vehicle,sy*tramWidth+sx);
          if (t!=15) color=pgm_read_word(IntroAssets::vehiclePalettes[selectedCity]+t);
        }
        strip[dy*w+dx]=color;
      }
      display->draw16bitRGBBitmap(x,row,strip,w,rows);
    }
  }
};
