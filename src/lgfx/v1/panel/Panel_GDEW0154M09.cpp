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
#include "Panel_GDEW0154M09.hpp"
#include "../Bus.hpp"
#include "../platforms/common.hpp"
#include "../misc/pixelcopy.hpp"

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  static constexpr std::uint8_t Bayer[16] = { 8, 200, 40, 232, 72, 136, 104, 168, 56, 248, 24, 216, 120, 184, 88, 152 };

  Panel_GDEW0154M09::Panel_GDEW0154M09(void)
  {
    _cfg.dummy_read_bits = 0;
    _epd_mode = epd_mode_t::epd_quality;
    _auto_display = true;
  }

  Panel_GDEW0154M09::~Panel_GDEW0154M09(void)
  {
    if (_buf) heap_free(_buf);
  }

  color_depth_t Panel_GDEW0154M09::setColorDepth(color_depth_t depth)
  {
    _write_bits = 16;
    _read_bits = 16;
    _write_depth = color_depth_t::rgb565_2Byte;
    _read_depth = color_depth_t::rgb565_2Byte;
    return color_depth_t::rgb565_2Byte;
  }

  bool Panel_GDEW0154M09::init(bool use_reset)
  {
    pinMode(_cfg.pin_busy, pin_mode_t::input);

    Panel_Device::init(use_reset);

    int len = ((_cfg.panel_width + 7) & ~7) * _cfg.panel_height >> 3;
    if (_buf) heap_free(_buf);
    _buf = static_cast<std::uint8_t*>(heap_alloc_dma(len));
    memset(_buf, 255, len);

    _wait_busy();

    startWrite(true);

    for (std::uint8_t i = 0; auto cmds = getInitCommands(i); i++)
    {
      command_list(cmds);
    }

    setInvert(_invert);

    setRotation(_rotation);

    _range_old.top = 0;
    _range_old.left = 0;
    _range_old.right = _width - 1;
    _range_old.bottom = _height - 1;
    _exec_transfer(0x13, _range_old);
    _close_transfer();
    _range_new = _range_old;

    endWrite();

    return true;
  }

  void Panel_GDEW0154M09::beginTransaction(void)
  {
    _bus->beginTransaction();
    cs_control(false);
  }

  void Panel_GDEW0154M09::endTransaction(void)
  {
    _bus->endTransaction();
    cs_control(true);
  }

  void Panel_GDEW0154M09::waitDisplay(void)
  {
    _wait_busy();
  }

  bool Panel_GDEW0154M09::displayBusy(void)
  {
    return _cfg.pin_busy >= 0 && !gpio_in(_cfg.pin_busy);
  }

  void Panel_GDEW0154M09::display(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h)
  {
    if (0 < w && 0 < h)
    {
      _range_new.left   = std::min<std::int16_t>(_range_new.left  , x        );
      _range_new.right  = std::max<std::int16_t>(_range_new.right , x + w - 1);
      _range_new.top    = std::min<std::int16_t>(_range_new.top   , y        );
      _range_new.bottom = std::max<std::int16_t>(_range_new.bottom, y + h - 1);
    }
    if (_range_new.empty()) { return; }
    _close_transfer();
    _range_old = _range_new;
    while (millis() - _send_msec < _refresh_msec) delay(1);
    if (getEpdMode() == epd_mode_t::epd_quality)
    {
      _exec_transfer(0x13, _range_new, true);
      _wait_busy();
      _bus->writeCommand(0x12, 8);
      auto send_msec = millis();
      delay(300);
      while (millis() - send_msec < _refresh_msec) delay(1);
      _exec_transfer(0x10, _range_new, true);
    }
    _exec_transfer(0x13, _range_new);
    _range_new.top    = INT16_MAX;
    _range_new.left   = INT16_MAX;
    _range_new.right  = 0;
    _range_new.bottom = 0;

    _wait_busy();
    _bus->writeCommand(0x12, 8);
    _send_msec = millis();
  }

  void Panel_GDEW0154M09::setInvert(bool invert)
  {
    startWrite();
    _invert = invert;
    _wait_busy();
    _bus->writeCommand(0x50, 8);  // VCOM and DATA Interval Setting (CDI)
    _bus->writeData((invert ^ _cfg.invert) ? 0xC7 : 0xD7, 8);
    endWrite();
  }

  void Panel_GDEW0154M09::setSleep(bool flg)
  {
    if (flg)
    {
      startWrite();
      _bus->writeCommand(0x50, 8);  // VCOM and DATA Interval Setting (CDI)
      _bus->writeData((_invert ^ _cfg.invert) ? 0xE7 : 0xF7, 8); // without OLD data
      _wait_busy();
      _bus->writeCommand(0x02, 8); // Power OFF

      _bus->writeCommand(0x07, 8); // Deep Sleep(DSLP)
      _bus->writeData(   0xA5, 8); // Deep Sleep(DSLP)
      endWrite();
    }
    else
    {
      init(true);
    }
  }

  void Panel_GDEW0154M09::setPowerSave(bool flg)
  {
    startWrite();
    _bus->writeCommand(0x50, 8);
    if (flg)
    {
      _bus->writeData((_invert ^ _cfg.invert) ? 0xE7 : 0xF7, 8); // without OLD data
      _wait_busy();
      _bus->writeCommand(0x02, 8); // Power OFF(POF)
    }
    else
    {
      _bus->writeData((_invert ^ _cfg.invert) ? 0xC7 : 0xD7, 8); // without OLD data
      _wait_busy();
      _bus->writeCommand(0x04, 8); // Power ON(PON)
    }
    endWrite();
  }

  void Panel_GDEW0154M09::setRotation(std::uint_fast8_t r)
  {
    r &= 7;
    _rotation = r;
    _internal_rotation = ((r + _cfg.offset_rotation) & 3) | ((r & 4) ^ (_cfg.offset_rotation & 4));

    _width  = _cfg.panel_width;
    _height = _cfg.panel_height;
    if (_internal_rotation & 1) std::swap(_width, _height);
  }

  void Panel_GDEW0154M09::setWindow(std::uint_fast16_t xs, std::uint_fast16_t ys, std::uint_fast16_t xe, std::uint_fast16_t ye)
  {
    _xpos = xs;
    _ypos = ys;
    _xs = xs;
    _ys = ys;
    _xe = xe;
    _ye = ye;
  }

  void Panel_GDEW0154M09::drawPixelPreclipped(std::uint_fast16_t x, std::uint_fast16_t y, std::uint32_t rawcolor)
  {
    bool need_transaction = !getStartCount();
    if (need_transaction) startWrite();
    writeFillRectPreclipped(x, y, 1, 1, rawcolor);
    if (need_transaction) endWrite();
  }

  void Panel_GDEW0154M09::writeFillRectPreclipped(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h, std::uint32_t rawcolor)
  {
    std::uint32_t xs = x, xe = x + w - 1;
    std::uint32_t ys = y, ye = y + h - 1;
    _xs = xs;
    _ys = ys;
    _xe = xe;
    _ye = ye;
    _update_transferred_rect(xs, ys, xe, ye);

    swap565_t color;
    color.raw = rawcolor;
    std::uint32_t value = (color.R8() + (color.G8() << 1) + color.B8()) >> 2;

    y = ys;
    do
    {
      x = xs;
      std::uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
      auto btbl = &Bayer[(y & 3) << 2];
      do
      {
        bool flg = 256 <= value + btbl[x & 3];
        if (flg) _buf[idx >> 3] |=   0x80 >> (idx & 7);
        else     _buf[idx >> 3] &= ~(0x80 >> (idx & 7));
        ++idx;
      } while (++x <= xe);
    } while (++y <= ye);
  }

  void Panel_GDEW0154M09::writeImage(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h, pixelcopy_t* param, bool use_dma)
  {
    std::uint32_t xs = x, xe = x + w - 1;
    std::uint32_t ys = y, ye = y + h - 1;
    _update_transferred_rect(xs, ys, xe, ye);

    swap565_t readbuf[w];
    auto sx = param->src_x32;
    h += y;
    do
    {
      std::uint32_t prev_pos = 0, new_pos = 0;
      do
      {
        new_pos = param->fp_copy(readbuf, prev_pos, w, param);
        if (new_pos != prev_pos)
        {
          do
          {
            auto color = readbuf[prev_pos];
            _draw_pixel(x + prev_pos, y, (color.R8() + (color.G8() << 1) + color.B8()) >> 2);
          } while (new_pos != ++prev_pos);
        }
      } while (w != new_pos && w != (prev_pos = param->fp_skip(new_pos, w, param)));
      param->src_x32 = sx;
      param->src_y++;
    } while (++y < h);
  }

  void Panel_GDEW0154M09::writeBlock(std::uint32_t rawcolor, std::uint32_t length)
  {
    std::uint32_t xs = _xs;
    std::uint32_t xe = _xe;
    std::uint32_t ys = _ys;
    std::uint32_t ye = _ye;
    std::uint32_t xpos = _xpos;
    std::uint32_t ypos = _ypos;
    do
    {
      auto len = std::min<std::uint32_t>(length, xe + 1 - xpos);
      writeFillRectPreclipped(xpos, ypos, len, 1, rawcolor);
      xpos += len;
      if (xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
      length -= len;
    } while (length);
    _xs = xs;
    _xe = xe;
    _ys = ys;
    _ye = ye;
    _xpos = xpos;
    _ypos = ypos;
  }

  void Panel_GDEW0154M09::writePixels(pixelcopy_t* param, std::uint32_t length)
  {
    {
      std::uint32_t xs = _xs;
      std::uint32_t xe = _xe;
      std::uint32_t ys = _ys;
      std::uint32_t ye = _ye;
      _update_transferred_rect(xs, ys, xe, ye);
    }
    std::uint32_t xs   = _xs  ;
    std::uint32_t ys   = _ys  ;
    std::uint32_t xe   = _xe  ;
    std::uint32_t ye   = _ye  ;
    std::uint32_t xpos = _xpos;
    std::uint32_t ypos = _ypos;

    static constexpr uint32_t buflen = 16;
    swap565_t colors[buflen];
    int bufpos = buflen;
    do
    {
      if (bufpos == buflen) {
        param->fp_copy(colors, 0, std::min(length, buflen), param);
        bufpos = 0;
      }
      auto color = colors[bufpos++];
      _draw_pixel(xpos, ypos, (color.R8() + (color.G8() << 1) + color.B8()) >> 2);
      if (++xpos > xe)
      {
        xpos = xs;
        if (++ypos > ye)
        {
          ypos = ys;
        }
      }
    } while (--length);
    _xpos = xpos;
    _ypos = ypos;
  }

  void Panel_GDEW0154M09::readRect(std::uint_fast16_t x, std::uint_fast16_t y, std::uint_fast16_t w, std::uint_fast16_t h, void* dst, pixelcopy_t* param)
  {
    swap565_t readbuf[w];
    param->src_data = readbuf;
    std::int32_t readpos = 0;
    h += y;
    do
    {
      std::uint32_t idx = 0;
      do
      {
        readbuf[idx] = _read_pixel(x + idx, y) ? ~0u : 0;
      } while (++idx != w);
      param->src_x32 = 0;
      readpos = param->fp_copy(dst, readpos, readpos + w, param);
    } while (++y < h);
  }

  bool Panel_GDEW0154M09::_wait_busy(std::uint32_t timeout)
  {
    if (_cfg.pin_busy >= 0 && !gpio_in(_cfg.pin_busy))
    {
      std::uint32_t start_time = millis();
      do
      {
        if (millis() - start_time > timeout) return false;
        delay(1);
      } while (!gpio_in(_cfg.pin_busy));
    }
    return true;
  }

  void Panel_GDEW0154M09::_draw_pixel(std::int32_t x, std::int32_t y, std::uint32_t value)
  {
    std::uint_fast8_t r = _internal_rotation;
    if (r)
    {
      if (r & 1) { std::swap(x, y); }
      std::uint_fast8_t rb = 1 << r;
      if (rb & 0b11000110) { x = _cfg.panel_width - x - 1; }  // case 1:2:6:7:
      if (rb & 0b10011100) { y = _cfg.panel_height - y - 1; } // case 2:3:4:7:
    }
    std::uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
    bool flg = 256 <= value + Bayer[(x & 3) | (y & 3) << 2];
    if (flg) _buf[idx >> 3] |=   0x80 >> (idx & 7);
    else     _buf[idx >> 3] &= ~(0x80 >> (idx & 7));
  }

  bool Panel_GDEW0154M09::_read_pixel(std::int32_t x, std::int32_t y)
  {
    std::uint_fast8_t r = _internal_rotation;
    if (r)
    {
      if (r & 1) { std::swap(x, y); }
      std::uint_fast8_t rb = 1 << r;
      if (rb & 0b11000110) { x = _cfg.panel_width - x - 1; }  // case 1:2:6:7:
      if (rb & 0b10011100) { y = _cfg.panel_height - y - 1; } // case 2:3:4:7:
    }
    std::uint32_t idx = ((_cfg.panel_width + 7) & ~7) * y + x;
    return _buf[idx >> 3] & (0x80 >> (idx & 7));
  }

  void Panel_GDEW0154M09::_exec_transfer(std::uint32_t cmd, const range_rect_t& range, bool invert)
  {
    std::int32_t xs = range.left & ~7;
    std::int32_t xe = range.right & ~7;

    _wait_busy();

    //_bus->writeCommand(0x91, 8);
    //_bus->writeCommand(0x90, 8);
    _bus->writeCommand(0x91 | 0x90 << 8, 16);

    _bus->writeData(xs | xe << 8, 16);
    _bus->writeData(__builtin_bswap16(range.top) | __builtin_bswap16(range.bottom)<<16, 32);
    _bus->writeData(1, 8);

    _wait_busy();

    _bus->writeCommand(cmd, 8);
    std::int32_t w = ((xe - xs) >> 3) + 1;
    std::int32_t y = range.top;
    std::int32_t add = ((_cfg.panel_width + 7) & ~7) >> 3;
    auto b = &_buf[xs >> 3];
    if (invert)
    {
      b += y * add;
      do
      {
        std::int32_t i = 0;
        do
        {
          _bus->writeData(~b[i], 8);
        } while (++i != w);
        b += add;
      } while (++y <= range.bottom);
    }
    else
    {
      do
      {
        _bus->writeBytes(&b[add * y], w, true, true);
      } while (++y <= range.bottom);
    }
    /*
    range->top = INT_MAX;
    range->left = INT_MAX;
    range->right = 0;
    range->bottom = 0;
    //*/
  }

  void Panel_GDEW0154M09::_close_transfer(void)
  {
    if (_range_old.empty()) { return; }
    while (millis() - _send_msec < _refresh_msec) delay(1);
    _exec_transfer(0x10, _range_old);
    _range_old.top    = INT16_MAX;
    _range_old.left   = INT16_MAX;
    _range_old.right  = 0;
    _range_old.bottom = 0;

    _bus->wait();
  }

  void Panel_GDEW0154M09::_update_transferred_rect(std::uint32_t &xs, std::uint32_t &ys, std::uint32_t &xe, std::uint32_t &ye)
  {
    auto r = _internal_rotation;
    if (r & 1) { std::swap(xs, ys); std::swap(xe, ye); }
    switch (r) {
    default: break;
    case 1:  case 2:  case 6:  case 7:
      std::swap(xs, xe);
      xs = _cfg.panel_width - 1 - xs;
      xe = _cfg.panel_width - 1 - xe;
      break;
    }
    switch (r) {
    default: break;
    case 2: case 3: case 4: case 7:
      std::swap(ys, ye);
      ys = _cfg.panel_height - 1 - ys;
      ye = _cfg.panel_height - 1 - ye;
      break;
    }
    std::int32_t x1 = xs & ~7;
    std::int32_t x2 = (xe & ~7) + 7;

    if (_range_old.horizon.intersectsWith(x1, x2) && _range_old.vertical.intersectsWith(ys, ye))
    {
      _close_transfer();
    }
    _range_new.top    = std::min<std::int32_t>(ys, _range_new.top);
    _range_new.left   = std::min<std::int32_t>(x1, _range_new.left);
    _range_new.right  = std::max<std::int32_t>(x2, _range_new.right);
    _range_new.bottom = std::max<std::int32_t>(ye, _range_new.bottom);
  }

//----------------------------------------------------------------------------
 }
}
