#pragma once
// Hardware pins of the LilyGO T-Display-S3-Long (taken from LilyGO's official examples)

#define LCD_USB_QSPI_DREVER   1

#define SPI_FREQUENCY         32000000
#define TFT_SPI_MODE          SPI_MODE0
#define TFT_SPI_HOST          SPI2_HOST

#define TFT_WIDTH             180
#define TFT_HEIGHT            640
#define EXAMPLE_LCD_H_RES     TFT_WIDTH
#define EXAMPLE_LCD_V_RES     TFT_HEIGHT

#define SEND_BUF_SIZE         (28800 / 2)  // 16bit (RGB565)

#define TFT_QSPI_CS           12
#define TFT_QSPI_SCK          17
#define TFT_QSPI_D0           13
#define TFT_QSPI_D1           18
#define TFT_QSPI_D2           21
#define TFT_QSPI_D3           14
#define TFT_QSPI_RST          16
#define TFT_BL                1

#define TP_RST                2
#define TP_IRQ                11
#define TP_I2C_SDA            15
#define TP_I2C_SCL            10

#define PIN_BUTTON_BOOT       0
