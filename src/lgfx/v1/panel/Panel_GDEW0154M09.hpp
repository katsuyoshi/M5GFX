/*----------------------------------------------------------------------------/
  Lovyan GFX - Graphics library for embedded devices.

Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)
/----------------------------------------------------------------------------*/
#pragma once

#include "Panel_Device.hpp"
#include "../misc/range.hpp"

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  struct Panel_GDEW0154M09 : public Panel_Device
  {
    Panel_GDEW0154M09(void);
    virtual ~Panel_GDEW0154M09(void);

    void beginTransaction(void) override;
    void endTransaction(void) override;
    bool init(bool use_reset) override;

    color_depth_t setColorDepth(color_depth_t depth) override;

    void setRotation(std::uint_fast8_t r) override;
    void setInvert(bool invert) override;
    void setSleep(bool flg) override;
    void setPowerSave(bool flg) override;

    void waitDisplay(void) override;
    bool displayBusy(void) override;
    void display(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h) override;

    void writeBlock(std::uint32_t rawcolor, std::uint32_t len) override;
    void setWindow(std::uint_fast16_t xs, std::uint_fast16_t ys, std::uint_fast16_t xe, std::uint_fast16_t ye) override;
    void drawPixelPreclipped(std::uint_fast16_t x, std::uint_fast16_t y, std::uint32_t rawcolor) override;
    void writeFillRectPreclipped(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h, std::uint32_t rawcolor) override;
    void writeImage(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h, pixelcopy_t* param, bool use_dma) override;
    void writePixels(pixelcopy_t* param, std::uint32_t len) override;

    std::uint32_t readCommand(std::uint_fast8_t cmd, std::uint_fast8_t index = 0, std::uint_fast8_t length = 4) override { return 0; }
    std::uint32_t readData(std::uint_fast8_t index = 0, std::uint_fast8_t length = 4) override { return 0; }

    void readRect(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h, void* dst, pixelcopy_t* param) override;

  private:

    static constexpr unsigned long _refresh_msec = 320;
    std::uint8_t* _buf = nullptr;

    range_rect_t _range_new;
    range_rect_t _range_old;
    std::int32_t _xpos = 0;
    std::int32_t _ypos = 0;
    unsigned long _send_msec = 0;

    bool _wait_busy(std::uint32_t timeout = 1000);
    void _draw_pixel(std::int32_t x, std::int32_t y, std::uint32_t value);
    bool _read_pixel(std::int32_t x, std::int32_t y);
    void _exec_transfer(std::uint32_t cmd, const range_rect_t& range, bool invert = false);
    void _close_transfer(void);
    void _update_transferred_rect(std::uint32_t &xs, std::uint32_t &ys, std::uint32_t &xe, std::uint32_t &ye);

    const std::uint8_t* getInitCommands(std::uint8_t listno) const override
    {
      static constexpr std::uint8_t list0[] = {
          0x00,2,0xdf,0x0e,       //panel setting
          0x4D,1,0x55,            //FITIinternal code
          0xaa,1,0x0f,
          0xe9,1,0x02,
          0xb6,1,0x11,
          0xf3,1,0x0a,
          0x61,3,0xc8,0x00,0xc8,  //resolution setting
          0x60,1,0x00,            //Tcon setting
          0xe3,1,0x00,
          0x04,0,                 //Power on
          0x00,2 + CMD_INIT_DELAY,0xff,0x0e, 160,       //panel setting

          0x20,56,0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x21,42,0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x22,56,0x01, 0x84, 0x84, 0x83, 0x01, 0x01, 0x01,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x23,56,0x01, 0x44, 0x44, 0x43, 0x01, 0x01, 0x01,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x24,56,0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0xFF,0xFF, // end
      };
      switch (listno) {
      case 0: return list0;
      default: return nullptr;
      }
    }
  };

//----------------------------------------------------------------------------
 }
}
