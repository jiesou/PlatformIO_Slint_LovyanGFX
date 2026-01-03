// See: https://github.com/lovyan03/LovyanGFX/blob/master/examples/HowToUse/2_user_setting/2_user_setting.ino
#pragma once
#include <LovyanGFX.hpp>

// Example configuration for using LovyanGFX with custom settings on ESP32

/// Create a class derived from lgfx::LGFX_Device to implement custom settings.
class LGFX : public lgfx::LGFX_Device
{
    /*
      You can change the class name from "LGFX" to something else.
      If using with AUTODETECT, change "LGFX" to a different name as it's already used.
      Also, give unique names when using multiple panels simultaneously.
      Note: If you change the class name, the constructor name must also match.

      Naming suggestion: For clarity, if using ILI9341 via SPI on ESP32 DevKit-C,
      a name like "LGFX_DevKitC_SPI_ILI9341" matching the filename is recommended.
    //*/

    // Prepare an instance for the specific panel model.
    // lgfx::Panel_GC9A01      _panel_instance;
    // lgfx::Panel_GDEW0154M09 _panel_instance;
    // lgfx::Panel_HX8357B      _panel_instance;
    // lgfx::Panel_HX8357D      _panel_instance;
    // lgfx::Panel_ILI9163      _panel_instance;
    // lgfx::Panel_ILI9341      _panel_instance;
    // lgfx::Panel_ILI9342      _panel_instance;
    // lgfx::Panel_ILI9481      _panel_instance;
    // lgfx::Panel_ILI9486      _panel_instance;
    // lgfx::Panel_ILI9488      _panel_instance;
    // lgfx::Panel_IT8951       _panel_instance;
    // lgfx::Panel_RA8875       _panel_instance;
    // lgfx::Panel_SH110x       _panel_instance; // SH1106, SH1107
    // lgfx::Panel_SSD1306      _panel_instance;
    // lgfx::Panel_SSD1327      _panel_instance;
    // lgfx::Panel_SSD1331      _panel_instance;
    // lgfx::Panel_SSD1351      _panel_instance; // SSD1351, SSD1357
    // lgfx::Panel_SSD1963      _panel_instance;
    // lgfx::Panel_ST7735       _panel_instance;
    // lgfx::Panel_ST7735S      _panel_instance;
    lgfx::Panel_ST7789       _panel_instance;
    // lgfx::Panel_ST7796       _panel_instance;

    // Prepare an instance for the bus type used to connect the panel.
    lgfx::Bus_SPI _bus_instance; // SPI bus instance
                                 // lgfx::Bus_I2C         _bus_instance;   // I2C bus instance
    // lgfx::Bus_Parallel8   _bus_instance;   // 8-bit parallel bus instance

    // Prepare an instance for backlight control (Delete if not needed).
    lgfx::Light_PWM _light_instance;

    // Prepare an instance for the specific touchscreen model (Delete if not needed).
    lgfx::Touch_CST816S           _touch_instance;
    // lgfx::Touch_FT5x06 _touch_instance; // FT5206, FT5306, FT5406, FT6206, FT6236, FT6336, FT6436
    // lgfx::Touch_GSL1680E_800x480 _touch_instance; // GSL_1680E, 1688E, 2681B, 2682B
    // lgfx::Touch_GSL1680F_800x480 _touch_instance;
    // lgfx::Touch_GSL1680F_480x272 _touch_instance;
    // lgfx::Touch_GSLx680_320x320  _touch_instance;
    // lgfx::Touch_GT911             _touch_instance;
    // lgfx::Touch_STMPE610          _touch_instance;
    // lgfx::Touch_TT21xxx           _touch_instance; // TT21100
    // lgfx::Touch_XPT2046           _touch_instance;

public:
    // Create constructor and perform various settings here.
    // If the class name was changed, ensure the constructor name matches.
    LGFX(void)
    {
        {                                      // Bus control settings.
            auto cfg = _bus_instance.config(); // Get the bus configuration structure.

            // SPI bus settings
            cfg.spi_host = SPI2_HOST; // Select SPI: ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
            // Note: VSPI_HOST/HSPI_HOST are deprecated in newer ESP-IDF. Use SPI2_HOST/SPI3_HOST if errors occur.
            cfg.spi_mode = 0;                  // SPI communication mode (0 ~ 3)
            cfg.freq_write = 80000000;         // SPI clock for transmission (Max 80MHz, rounded to integer divisor of 80MHz)
            cfg.freq_read = 16000000;          // SPI clock for receiving
            cfg.spi_3wire = true;              // Set to true if receiving via MOSI pin
            cfg.use_lock = true;               // Set to true to use transaction locks
            cfg.dma_channel = SPI_DMA_CH_AUTO; // DMA channel (0=None / 1=1ch / 2=2ch / SPI_DMA_CH_AUTO=Auto)
            // Note: SPI_DMA_CH_AUTO is recommended in newer ESP-IDF.
            cfg.pin_sclk = 12; // SPI SCLK pin number
            cfg.pin_mosi = 11; // SPI MOSI pin number
            cfg.pin_miso = -1; // SPI MISO pin number (-1 = disable)
            cfg.pin_dc = 14;   // SPI D/C pin number  (-1 = disable)
                               // If sharing SPI bus with SD card, MISO must be configured.
                               //*/
                               /*
                               // I2C bus settings
                                     cfg.i2c_port    = 0;          // Select I2C port (0 or 1)
                                     cfg.freq_write  = 400000;     // Clock for transmission
                                     cfg.freq_read   = 400000;     // Clock for receiving
                                     cfg.pin_sda     = 21;         // SDA pin number
                                     cfg.pin_scl     = 22;         // SCL pin number
                                     cfg.i2c_addr    = 0x3C;       // I2C device address
                               //*/
                               /*
                               // 8-bit parallel bus settings
                                     cfg.i2s_port = I2S_NUM_0;     // Select I2S port (I2S_NUM_0 or 1) (Uses ESP32 I2S LCD mode)
                                     cfg.freq_write = 20000000;    // Clock for transmission (Max 20MHz)
                                     cfg.pin_wr =  4;              // WR pin number
                                     cfg.pin_rd =  2;              // RD pin number
                                     cfg.pin_rs = 15;              // RS (D/C) pin number
                                     cfg.pin_d0 = 12;              // D0 pin number
                                     cfg.pin_d1 = 13;              // D1 pin number
                                     cfg.pin_d2 = 26;              // D2 pin number
                                     cfg.pin_d3 = 25;              // D3 pin number
                                     cfg.pin_d4 = 17;              // D4 pin number
                                     cfg.pin_d5 = 16;              // D5 pin number
                                     cfg.pin_d6 = 27;              // D6 pin number
                                     cfg.pin_d7 = 14;              // D7 pin number
                               //*/

            _bus_instance.config(cfg);              // Apply settings to the bus.
            _panel_instance.setBus(&_bus_instance); // Connect the bus to the panel.
        }

        {                                        // Display panel control settings.
            auto cfg = _panel_instance.config(); // Get the panel configuration structure.

            cfg.pin_cs = 10;   // CS pin number  (-1 = disable)
            cfg.pin_rst = 13;  // RST pin number (-1 = disable)
            cfg.pin_busy = -1; // BUSY pin number (-1 = disable)

            // Note: General defaults are set per panel. Comment out unknown items to test.

            cfg.panel_width = 240;    // Actual displayable width
            cfg.panel_height = 280;   // Actual displayable height
            cfg.offset_x = 0;         // Panel X offset
            cfg.offset_y = 20;         // Panel Y offset
            cfg.offset_rotation = 0;  // Rotation offset 0~7 (4~7 are upside down)
            cfg.dummy_read_pixel = 8; // Dummy read bits before pixel readout
            cfg.dummy_read_bits = 1;  // Dummy read bits before non-pixel data readout
            cfg.readable = true;      // Set to true if data readout is supported
            cfg.invert = false;       // Set to true if panel colors are inverted
            cfg.rgb_order = false;    // Set to true if Red and Blue are swapped
            cfg.dlen_16bit = false;   // Set to true for panels using 16-bit units for SPI/Parallel
            cfg.bus_shared = true;    // Set to true if sharing bus with SD card (enables bus control for drawJpgFile, etc.)

            // Set only if display is misaligned on drivers with variable pixel counts (e.g., ST7735, ILI9163).
            //    cfg.memory_width     =   240;  // Max width supported by driver IC
            //    cfg.memory_height    =   320;  // Max height supported by driver IC

            _panel_instance.config(cfg);
        }

        //*
        {                                        // Backlight control settings (Delete if not needed).
            auto cfg = _light_instance.config(); // Get backlight configuration structure.

            cfg.pin_bl = 16;     // Backlight pin number
            cfg.invert = false;  // True to invert backlight brightness
            cfg.freq = 44100;    // PWM frequency for backlight
            cfg.pwm_channel = 7; // PWM channel number

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance); // Connect backlight to the panel.
        }
        //*/

        //*
        { // Touchscreen control settings (Delete if not needed).
            auto cfg = _touch_instance.config();

            cfg.x_min = 0;           // Min X value from touchscreen (raw)
            cfg.x_max = 239;         // Max X value from touchscreen (raw)
            cfg.y_min = 0;           // Min Y value from touchscreen (raw)
            cfg.y_max = 319;         // Max Y value from touchscreen (raw)
            cfg.pin_int = 38;        // INT pin number
            cfg.bus_shared = true;   // True if sharing the same bus as the display
            cfg.offset_rotation = 0; // Adjust if touch orientation doesn't match display (0~7)

            // For SPI connection
            // cfg.spi_host = VSPI_HOST; // Select SPI (HSPI_HOST or VSPI_HOST)
            // cfg.freq = 1000000;       // SPI clock frequency
            // cfg.pin_sclk = 18;        // SCLK pin number
            // cfg.pin_mosi = 23;        // MOSI pin number
            // cfg.pin_miso = 19;        // MISO pin number
            // cfg.pin_cs = 5;           // CS pin number

            // For I2C connection
            cfg.i2c_port = 0;    // Select I2C (0 or 1)
            cfg.i2c_addr = 0x15; // I2C device address
            cfg.pin_sda = 8;    // SDA pin number
            cfg.pin_scl = 9;    // SCL pin number
            cfg.freq = 400000;   // I2C clock frequency

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance); // Connect touchscreen to the panel.
        }
        //*/

        setPanel(&_panel_instance); // Set the panel to be used.
    }
};

// Create an instance of the prepared class.
LGFX gfx;
