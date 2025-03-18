#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dw1000_spi, LOG_LEVEL_DBG);

// SPI Configuration
#define DW1000_SPI_FREQUENCY 2000000                    // 2 MHz
#define DW1000_SPI_MODE (SPI_MODE_CPOL | SPI_MODE_CPHA) // SPI Mode 0 (CPOL = 0, CPHA = 0)

// SPI device and configuration
const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi2));
const struct spi_config spi_cfg = {
    .frequency = DW1000_SPI_FREQUENCY,
    .operation = SPI_WORD_SET(8) | DW1000_SPI_MODE,
    .slave = 0,
    .cs = NULL, // CS handled via GPIO
};

#define SPIOP SPI_WORD_SET(8) | SPI_TRANSFER_MSB
struct spi_dt_spec spispec = SPI_DT_SPEC_GET(DT_NODELABEL(ieee802154), SPIOP, 0);

// GPIO for Chip Select (CS) and reset
const struct gpio_dt_spec cs_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(spi2), cs_gpios);
const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(ieee802154), reset_gpios);

// SPI write
int dw1000_spi_write(const struct device *spi_dev, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t tx_buf[1 + len];       // Register header + data
    tx_buf[0] = 0x80 | reg;        // Op. bit + address
    memcpy(&tx_buf[1], data, len); // Data to be written

    struct spi_buf tx_bufs[] = {
        {.buf = tx_buf, .len = sizeof(tx_buf)},
    };

    struct spi_buf_set tx = {.buffers = tx_bufs, .count = 1};

    gpio_pin_set_dt(&cs_gpio, 0); // Assert CS
    int ret = spi_write_dt(&spispec, &tx);
    gpio_pin_set_dt(&cs_gpio, 1); // Deassert CS

    if (ret)
    {
        LOG_ERR("SPI write failed: %d", ret);
    }
    return ret;
}

int dw1000_spi_read(const struct device *spi_dev, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t tx_buf[2];
    tx_buf[0] = reg & 0x3F; // Read operation: MSB=0
    tx_buf[1] = 0;

    struct spi_buf tx_bufs[] = {
        {.buf = &tx_buf, .len = 1}, // Send register address
    };
    struct spi_buf rx_bufs[] = {
        {.buf = data, .len = len}, // Receive data
    };

    struct spi_buf_set tx = {.buffers = tx_bufs, .count = 1};
    struct spi_buf_set rx = {.buffers = rx_bufs, .count = 1};

    gpio_pin_set_dt(&cs_gpio, 0); // Assert CS
    int ret = spi_transceive_dt(&spispec, &tx, &rx);
    gpio_pin_set_dt(&cs_gpio, 1); // Deassert CS

    if (ret)
    {
        LOG_ERR("SPI read failed: %d", ret);
        return ret;
    }

    // Log the received data
    LOG_INF("SPI read successful. Register 0x%02X: ", reg);
    for (size_t i = 1; i < len; i++)
    {
        LOG_INF("Byte %zu: 0x%02X", i - 1, data[i]);
    }

    return 0;
}

int main(void)
{
    int ret;

    // Check if SPI device is ready
    if (!device_is_ready(spi_dev))
    {
        LOG_ERR("SPI device not ready");
        return 1;
    }

    // Configure CS GPIO
    if (!device_is_ready(cs_gpio.port))
    {
        LOG_ERR("CS GPIO device not ready");
        return 1;
    }
    ret = gpio_pin_configure_dt(&cs_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret)
    {
        LOG_ERR("Failed to configure CS GPIO: %d", ret);
        return 1;
    }
    LOG_INF("CS GPIO configured...");

    gpio_pin_set_dt(&reset_gpio, 0);
    // LOG_INF("Starting SPI transmission...");
    gpio_pin_set_dt(&reset_gpio, 1);
    while (1)
    {

        // write the data to transmit
        // address: TX_BUFFER = 0x09
        uint8_t tx_data[3] = {0x01, 0x02, 0x03}; // Data to be transmitted
        uint8_t rx_data[4] = {0};

        ret = dw1000_spi_write(spi_dev, 0x09, tx_data, sizeof(tx_data));
        if (ret == 0)
        {
            LOG_INF("SPI write to register TX_BUFFER successful");
        }

        // configure transmit frame
        // address: TRANSMIT FRAME CONTROL(TX_FCTRL) = 0x08
        // Configure preamble length (64), data rate (6.8Mbps), PRF(16MHz)
        uint8_t tfc_data[3] = {0x07, 0xA0, 0x02};

        ret = dw1000_spi_write(spi_dev, 0x08, tfc_data, sizeof(tfc_data));
        if (ret == 0)
        {
            LOG_INF("SPI write to register TX_FCTRL successful");
        }

        // start transmission
        // address: SYSTEM CONTROL (SYS_CTRL) = 0x0D
        // Write the TXSTRT bit
        LOG_INF("Starting SPI read in SYS_CTRL register so we don't modify any unwanted bits");
        dw1000_spi_read(spi_dev, 0x0D, rx_data, sizeof(rx_data));
        uint8_t go_data[4];
        for (int i = 0; i < 3; ++i)
        {
            go_data[i] = rx_data[i];
        }
        go_data[0] = go_data[0] | 0x2;

        ret = dw1000_spi_write(spi_dev, 0x0D, go_data, sizeof(go_data));
        if (ret == 0)
        {
            LOG_INF("SPI write to register SYS_CTRL successful");
        }

        // wait for confirmation that the transmission ended
        // address: SYSTEM EVENT (SYS_STATUS) = 0x0F
        // Check the TXFRS bit (bit 7)
        LOG_INF("Waiting for confirmation that the transmission ended...");
        LOG_INF("ready bit?");
        dw1000_spi_read(spi_dev, 0x0F, rx_data, sizeof(rx_data));
        while ((rx_data[0] & 0x80) == 0)
        {
            LOG_INF("ready bit?");
            dw1000_spi_read(spi_dev, 0x0F, rx_data, sizeof(rx_data));
        }

        LOG_INF("Transfer successful!");
        LOG_INF("__________________________________________________________________");

        for (int i = 0; i < 10000; ++i)
            ;
    }

    return 0;
}
