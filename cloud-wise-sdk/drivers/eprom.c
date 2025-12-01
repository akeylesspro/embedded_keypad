#include "eprom.h"

#include "FreeRTOS.h"
#include "hal/hal_boards.h"
#include "nrfx_log.h"
#include "nrfx_spi.h"
#include "semphr.h"
#include "task.h"
#include "logic/ignition.h"

#include <stdio.h>
#include <string.h>

static const nrfx_spi_t m_spi1        = NRFX_SPI_INSTANCE(1);
const nrfx_spi_t       *hal_eprom_spi = &m_spi1;

static volatile bool spi_xfer_done;

xSemaphoreHandle eprom_semaphore;

extern const nrfx_spi_t *hal_eprom_spi;

void eprom_spi_event_handler(nrfx_spi_evt_t const *p_event, void *p_context)
{
    spi_xfer_done = true;
    // NRF_LOG_DEBUG("Transfer completed.");
}

static void eprom_spi_init(nrfx_spi_evt_handler_t handler)
{
    nrfx_err_t err_code;

    nrfx_spi_config_t spi_config = NRFX_SPI_DEFAULT_CONFIG;
   //Ran swap between sck and miso
    spi_config.sck_pin  = HAL_SPI1_CLK;
    //spi_config.sck_pin  = HAL_SPI1_MISO;

    spi_config.ss_pin   = HAL_SPI1_SS;
    spi_config.mosi_pin = HAL_SPI1_MOSI;

    spi_config.miso_pin = HAL_SPI1_MISO;
    //spi_config.miso_pin = HAL_SPI1_CLK;

    spi_config.mode      = NRF_SPI_MODE_0;
    spi_config.frequency = NRF_SPI_FREQ_8M;

    err_code = nrfx_spi_init(&m_spi1, &spi_config, eprom_spi_event_handler, NULL);
    if (NRFX_SUCCESS != err_code) {
        NRFX_LOG_ERROR("%s nrfx_spi_init failed: %s", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    }
    APP_ERROR_CHECK(err_code);
}

void eprom_init(void)
{
    eprom_spi_init(eprom_spi_event_handler);
    eprom_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(eprom_semaphore);
}

uint16_t eprom_read_manufacture_id(void)
{
    uint8_t tx_buf[4];
    uint8_t rx_buf[4];

    uint16_t ret;

    xSemaphoreTake(eprom_semaphore, portMAX_DELAY);

    memset(tx_buf, 0x00, 4);
    memset(rx_buf, 0x00, 4);

    spi_xfer_done = false;

    tx_buf[0] = READ_COMMAND;
    tx_buf[1] = MANUFACTURE_ADDRESS;

    nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TRX(tx_buf, 4, rx_buf, 4);

    APP_ERROR_CHECK(nrfx_spi_xfer(&m_spi1, &xfer, 0));

    while (!spi_xfer_done) {
        __WFE();
    }

    ret = rx_buf[2] << 8;
    ret |= rx_buf[3];

    xSemaphoreGive(eprom_semaphore);

    return ret;
}

uint32_t eprom_read_device_id(void)
{
    uint8_t tx_buf[6];
    uint8_t rx_buf[6];

    uint32_t ret;

    xSemaphoreTake(eprom_semaphore, portMAX_DELAY);

    memset(tx_buf, 0x00, 6);
    memset(rx_buf, 0x00, 6);

    spi_xfer_done = false;

    tx_buf[0] = READ_COMMAND;
    tx_buf[1] = DEVICE_ADDRESS;

    nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TRX(tx_buf, 5, rx_buf, 6);

    APP_ERROR_CHECK(nrfx_spi_xfer(&m_spi1, &xfer, 0));

    while (!spi_xfer_done) {
        __WFE();
    }

    ret = rx_buf[2] << 24;
    ret |= rx_buf[3] << 16;
    ret |= rx_buf[4] << 8;
    ret |= rx_buf[5];

    xSemaphoreGive(eprom_semaphore);

    return ret;
}

uint8_t *eprom_read(uint8_t *buffer, uint8_t address, uint8_t length)
{
    uint8_t tx_buf[4];

    xSemaphoreTake(eprom_semaphore, portMAX_DELAY);

    spi_xfer_done = false;

    tx_buf[0] = READ_COMMAND;
    tx_buf[1] = address;

    nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TRX(tx_buf, (length + 2), buffer, (length + 2));

    APP_ERROR_CHECK(nrfx_spi_xfer(&m_spi1, &xfer, 0));

    while (!spi_xfer_done) {
        __WFE();
    }

    xSemaphoreGive(eprom_semaphore);

    return (buffer + 2);
}

void eprom_write(uint8_t *buffer, uint8_t address, uint8_t length)
{
    static uint8_t rx_dummy_buffer[20];
    uint8_t        tx_buf[20];

    if (length > 16) {
        return;
    }

    xSemaphoreTake(eprom_semaphore, portMAX_DELAY);

    // write enable
    spi_xfer_done             = false;
    tx_buf[0]                 = WRITE_ENABLE_COMMAND;
    nrfx_spi_xfer_desc_t xfer = NRFX_SPI_XFER_TRX(tx_buf, (2), tx_buf, (2));
    APP_ERROR_CHECK(nrfx_spi_xfer(&m_spi1, &xfer, 0));

    while (!spi_xfer_done) {
        __WFE();
    }

    vTaskDelay(1);

    // write buffer
    spi_xfer_done = false;
    tx_buf[0]     = WRITE_COMMAND;
    tx_buf[1]     = address;

    memcpy(tx_buf + 2, buffer, length);
    nrfx_spi_xfer_desc_t xfer1 = NRFX_SPI_XFER_TRX(tx_buf, (length + 2), rx_dummy_buffer, (length + 2));

    APP_ERROR_CHECK(nrfx_spi_xfer(&m_spi1, &xfer1, 0));

    while (!spi_xfer_done) {
        __WFE();
    }
    vTaskDelay(10);

    // write enable
    spi_xfer_done              = false;
    tx_buf[0]                  = WRITE_DISABLE_COMMAND;
    nrfx_spi_xfer_desc_t xfer2 = NRFX_SPI_XFER_TRX(tx_buf, (2), tx_buf, (2));
    APP_ERROR_CHECK(nrfx_spi_xfer(&m_spi1, &xfer2, 0));

    while (!spi_xfer_done) {
        __WFE();
    }
    vTaskDelay(1);

    xSemaphoreGive(eprom_semaphore);
}