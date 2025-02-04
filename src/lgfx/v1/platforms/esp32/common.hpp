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

#include "../../misc/DataWrapper.hpp"
#include "../../misc/enum.hpp"
#include "../../../utility/result.hpp"

#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

namespace lgfx
{
 inline namespace v1
 {
//----------------------------------------------------------------------------

  __attribute__ ((unused)) static inline unsigned long millis(void) { return (unsigned long) (esp_timer_get_time() / 1000ULL); }
  __attribute__ ((unused)) static inline unsigned long micros(void) { return (unsigned long) (esp_timer_get_time()); }
  __attribute__ ((unused)) static inline void delayMicroseconds(std::uint32_t us) { ets_delay_us(us); }
  __attribute__ ((unused)) static inline void delay(std::uint32_t ms)
  {
    std::uint32_t time = micros();
    vTaskDelay( (ms >= portTICK_PERIOD_MS) ? (ms / portTICK_PERIOD_MS - 1) : 0);
    if (ms != 0 && ms < portTICK_PERIOD_MS*8)
    {
      ms *= 1000;
      time = micros() - time;
      if (time < ms)
      {
        ets_delay_us(ms - time);
      }
    }
  }

  static inline void* heap_alloc(      size_t length) { return heap_caps_malloc(length, MALLOC_CAP_8BIT);  }
  static inline void* heap_alloc_dma(  size_t length) { return heap_caps_malloc((length + 3) & ~3, MALLOC_CAP_DMA);  }
  static inline void* heap_alloc_psram(size_t length) { return heap_caps_malloc(length, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);  }
  static inline void heap_free(void* buf) { heap_caps_free(buf); }

  enum pin_mode_t
  { output
  , input
  , input_pullup
  , input_pulldown
  };

  void pinMode(std::int_fast16_t pin, pin_mode_t mode);
  inline void lgfxPinMode(std::int_fast16_t pin, pin_mode_t mode)
  {
    pinMode(pin, mode);
  }

  static inline volatile std::uint32_t* get_gpio_hi_reg(std::int_fast8_t pin) { return (pin & 32) ? &GPIO.out1_w1ts.val : &GPIO.out_w1ts; }
//static inline volatile std::uint32_t* get_gpio_hi_reg(std::int_fast8_t pin) { return (volatile uint32_t*)((pin & 32) ? 0x60004014 : 0x60004008) ; } // workaround Eratta
  static inline volatile std::uint32_t* get_gpio_lo_reg(std::int_fast8_t pin) { return (pin & 32) ? &GPIO.out1_w1tc.val : &GPIO.out_w1tc; }
//static inline volatile std::uint32_t* get_gpio_lo_reg(std::int_fast8_t pin) { return (volatile uint32_t*)((pin & 32) ? 0x60004018 : 0x6000400C) ; }
  static inline void gpio_hi(std::int_fast8_t pin) { if (pin >= 0) *get_gpio_hi_reg(pin) = 1 << (pin & 31); } // ESP_LOGI("LGFX", "gpio_hi: %d", pin); }
  static inline void gpio_lo(std::int_fast8_t pin) { if (pin >= 0) *get_gpio_lo_reg(pin) = 1 << (pin & 31); } // ESP_LOGI("LGFX", "gpio_lo: %d", pin); }
  static inline bool gpio_in(std::int_fast8_t pin) { return ((pin & 32) ? GPIO.in1.data : GPIO.in) & (1 << (pin & 31)); }

  std::uint32_t getApbFrequency(void);
  std::uint32_t FreqToClockDiv(std::uint32_t fapb, std::uint32_t hz);

//----------------------------------------------------------------------------

#if defined (ARDUINO)
 #if defined (FS_H)

  struct FileWrapper : public DataWrapper
  {
private:
#if defined (_SD_H_)
    bool _check_need_transaction(void) const { return _fs == &SD; }
#elif defined (_SPIFFS_H_)
    bool _check_need_transaction(void) const { return _fs != &SPIFFS; }
#else
    bool _check_need_transaction(void) const { return false; }
#endif

public:
    FileWrapper() : DataWrapper()
    {
#if defined (_SD_H_)
      _fs = &SD;
#elif defined (_SPIFFS_H_)
      _fs = &SPIFFS;
#else
      _fs = nullptr;
#endif
      need_transaction = _check_need_transaction();
      _fp = nullptr;
    }

    fs::FS* _fs;
    fs::File *_fp;
    fs::File _file;

    FileWrapper(fs::FS& fs, fs::File* fp = nullptr) : DataWrapper(), _fs(&fs), _fp(fp) { need_transaction = _check_need_transaction(); }
    void setFS(fs::FS& fs) {
      _fs = &fs;
      need_transaction = _check_need_transaction();
    }

    bool open(fs::FS& fs, const char* path)
    {
      setFS(fs);
      return open(path);
    }
    bool open(const char* path) override
    {
      _file = _fs->open(path, "r");
      _fp = &_file;
      return _file;
    }
    int read(std::uint8_t *buf, std::uint32_t len) override { return _fp->read(buf, len); }
    void skip(std::int32_t offset) override { seek(offset, SeekCur); }
    bool seek(std::uint32_t offset) override { return seek(offset, SeekSet); }
    bool seek(std::uint32_t offset, SeekMode mode) { return _fp->seek(offset, mode); }
    void close(void) override { if (_fp) _fp->close(); }
    std::int32_t tell(void) override { return _fp->position(); }
  };
 #else
  // dummy
  struct FileWrapper : public DataWrapper
  {
    FileWrapper() : DataWrapper()
    {
      need_transaction = true;
    }
    void* _fp;

    template <typename T>
    void setFS(T& fs) {}

    bool open(const char* path, const char* mode) { return false; }
    int read(std::uint8_t *buf, std::uint32_t len) override { return false; }
    void skip(std::int32_t offset) override { }
    bool seek(std::uint32_t offset) override { return false; }
    bool seek(std::uint32_t offset, int origin) { return false; }
    void close() override { }
    std::int32_t tell(void) override { return 0; }
  };

 #endif
#else // ESP-IDF

  struct FileWrapper : public DataWrapper
  {
    FileWrapper() : DataWrapper()
    {
      need_transaction = true;
    }
    FILE* _fp;
    bool open(const char* path) override { return (_fp = fopen(path, "r")); }
    int read(std::uint8_t *buf, std::uint32_t len) override { return fread((char*)buf, 1, len, _fp); }
    void skip(std::int32_t offset) override { seek(offset, SEEK_CUR); }
    bool seek(std::uint32_t offset) override { return seek(offset, SEEK_SET); }
    bool seek(std::uint32_t offset, int origin) { return fseek(_fp, offset, origin); }
    void close() override { if (_fp) fclose(_fp); }
    std::int32_t tell(void) override { return ftell(_fp); }
  };

#endif

//----------------------------------------------------------------------------

#if defined (ARDUINO) && defined (Stream_h)

  struct StreamWrapper : public DataWrapper
  {
    void set(Stream* src, std::uint32_t length = ~0) { _stream = src; _length = length; _index = 0; }

    int read(std::uint8_t *buf, std::uint32_t len) override {
      len = std::min<std::uint32_t>(len, _stream->available());
      if (len > _length - _index) { len = _length - _index; }
      _index += len;
      return _stream->readBytes((char*)buf, len);
    }
    void skip(std::int32_t offset) override { if (0 < offset) { char dummy[offset]; _stream->readBytes(dummy, offset); _index += offset; } }
    bool seek(std::uint32_t offset) override { if (offset < _index) { return false; } skip(offset - _index); return true; }
    void close() override { }
    std::int32_t tell(void) override { return _index; }

  protected:
    Stream* _stream;
    std::uint32_t _index;
    std::uint32_t _length = 0;
  };

#endif

//----------------------------------------------------------------------------

  namespace spi
  {
    void init(int spi_host, int spi_sclk, int spi_miso, int spi_mosi, int dma_channel);
    void init(int spi_host, int spi_sclk, int spi_miso, int spi_mosi);
    void release(int spi_host);
    void beginTransaction(int spi_host, std::uint32_t freq, int spi_mode = 0);
    void beginTransaction(int spi_host);
    void endTransaction(int spi_host);
    void writeBytes(int spi_host, const std::uint8_t* data, std::size_t length);
    void readBytes(int spi_host, std::uint8_t* data, std::size_t length);
  }

  namespace i2c
  {
    static constexpr std::uint32_t I2C_DEFAULT_FREQ = 400000;

    cpp::result<void, error_t> init(int i2c_port, int pin_sda, int pin_scl);
    cpp::result<void, error_t> release(int i2c_port);
    cpp::result<void, error_t> restart(int i2c_port, int i2c_addr, std::uint32_t freq, bool read = false);
    cpp::result<void, error_t> beginTransaction(int i2c_port, int i2c_addr, std::uint32_t freq, bool read = false);
    cpp::result<void, error_t> endTransaction(int i2c_port);
    cpp::result<void, error_t> writeBytes(int i2c_port, const std::uint8_t *data, std::size_t length);
    cpp::result<void, error_t> readBytes(int i2c_port, std::uint8_t *data, std::size_t length);

//--------

    cpp::result<void, error_t> transactionWrite(int i2c_port, int addr, const std::uint8_t *writedata, std::uint8_t writelen, std::uint32_t freq = I2C_DEFAULT_FREQ);
    cpp::result<void, error_t> transactionRead(int i2c_port, int addr, std::uint8_t *readdata, std::uint8_t readlen, std::uint32_t freq = I2C_DEFAULT_FREQ);
    cpp::result<void, error_t> transactionWriteRead(int i2c_port, int addr, const std::uint8_t *writedata, std::uint8_t writelen, std::uint8_t *readdata, std::size_t readlen, std::uint32_t freq = I2C_DEFAULT_FREQ);

    cpp::result<std::uint8_t, error_t> registerRead8(int i2c_port, int addr, std::uint8_t reg, std::uint32_t freq = I2C_DEFAULT_FREQ);
    cpp::result<void, error_t> registerWrite8(int i2c_port, int addr, std::uint8_t reg, std::uint8_t data, std::uint8_t mask = 0, std::uint32_t freq = I2C_DEFAULT_FREQ);

    inline cpp::result<void, error_t> registerRead(int i2c_port, int addr, std::uint8_t reg, std::uint8_t* data, std::size_t len, std::uint32_t freq = I2C_DEFAULT_FREQ)
    {
      return transactionWriteRead(i2c_port, addr, &reg, 1, data, len, freq);
    }
    inline cpp::result<void, error_t> bitOn(int i2c_port, int addr, std::uint8_t reg, std::uint8_t bit, std::uint32_t freq = I2C_DEFAULT_FREQ)
    {
      return registerWrite8(i2c_port, addr, reg, bit, ~0, freq);
    }
    inline cpp::result<void, error_t> bitOff(int i2c_port, int addr, std::uint8_t reg, std::uint8_t bit, std::uint32_t freq = I2C_DEFAULT_FREQ)
    {
      return registerWrite8(i2c_port, addr, reg, 0, ~bit, freq);
    }
  }

//----------------------------------------------------------------------------
 }
}
