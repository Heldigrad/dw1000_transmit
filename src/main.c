//*********************************************/
// RX
//*********************************************/

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dw1000_spi, LOG_LEVEL_DBG);

#define SLEEP_TIME_MS 1000

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
    LOG_INF("SPI read successful. Data from register 0x%X: ", reg);
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
    k_msleep(2);
    gpio_pin_set_dt(&reset_gpio, 1);
    k_msleep(5);

    while (1)
    {
        LOG_INF("\n[RX]");

        // Read and verify Device ID
        uint8_t dev_id[4] = {0};
        dw1000_spi_read(spi_dev, 0x00, dev_id, sizeof(dev_id));

        uint8_t chan_ctrl[4] = {0x25, 0x00, 0x00, 0x00}; // Channel 5, Preamble Code 3
        dw1000_spi_write(spi_dev, 0x1F, chan_ctrl, sizeof(chan_ctrl));

        // Configure RX Frame Control Register (TX settings must match this!)
        uint8_t rx_fctrl[5] = {0x0C, 0x00, 0x42, 0x00, 0x00}; // Frame length, 6.8Mbps, 16MHz PRF
        dw1000_spi_write(spi_dev, 0x08, rx_fctrl, sizeof(rx_fctrl));

        // Enable RX Auto-Reenable BEFORE enabling RX
        uint8_t sys_cfg[4] = {0x20, 0x00, 0x00, 0x00}; // Enable RX auto-reenable
        dw1000_spi_write(spi_dev, 0x04, sys_cfg, sizeof(sys_cfg));

        // Clear RX Status Flags before polling
        uint8_t clear_status[4] = {0xE0, 0x00, 0x00, 0x00}; // Clear RX flags
        dw1000_spi_write(spi_dev, 0x0F, clear_status, sizeof(clear_status));

        // Enable Receiver
        uint8_t sys_ctrl[1] = {0x01}; // Enable RX
        dw1000_spi_write(spi_dev, 0x0D, sys_ctrl, sizeof(sys_ctrl));

        // Wait for a valid frame (RXFCG bit in SYS_STATUS)
        uint8_t sys_status[4] = {0};
        do
        {
            dw1000_spi_read(spi_dev, 0x0F, sys_status, sizeof(sys_status));
        } while (!(sys_status[0] & 0x40)); // Check RXFCG bit

        // Read received data
        uint8_t rx_data[10]; // Adjust based on expected frame size
        dw1000_spi_read(spi_dev, 0x11, rx_data, sizeof(rx_data));

        LOG_INF("Received Data:");
        for (size_t i = 0; i < sizeof(rx_data); i++)
        {
            LOG_INF("Byte %zu: 0x%02X", i, rx_data[i]);
        }

        // Clear RX flags
        clear_status[0] = 0xE0;
        dw1000_spi_write(spi_dev, 0x0F, clear_status, sizeof(clear_status));

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
