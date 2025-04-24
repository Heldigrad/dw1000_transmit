//*********************************************/
// TX
//*********************************************/

#include "C:\Users\agape\Documents\LICENTA\dw1000_app\DW1000-driver\includes.h"

int main(void)
{
    if (check_devices_ready())
    {
        LOG_ERR("Devices not ready!");
        return 1;
    }
    gpio_pin_configure_dt(&reset_gpio, GPIO_OPEN_DRAIN | GPIO_OUTPUT);
    reset_devices();

    // soft reset
    // uint8_t softreset = 0x00;
    // dw1000_subwrite_u8(PMSC, 0x04, softreset);

    LOG_INF("\n[TX]");
    uint32_t dev_id;
    dw1000_read_u32(0x00, &dev_id);

    // generic_default_configs();
    // tx_default_configs();
    // additional_default_configs();

    // initialize();
    // configure();

    while (1)
    {
        LOG_INF("SIMPLE_TX");

        reset_devices();

        generic_default_configs();
        tx_default_configs();
        additional_default_configs();

        // Device ID
        uint32_t dev_id;
        dw1000_read_u32(0x00, &dev_id);

        // TX_BUFFER = 0x09
        uint32_t tx_data = 0xAABAAB;
        dw1000_write_u32(0x09, tx_data);

        // uint32_t reg32 = pdw1000local.txFCTRL | sizeof(tx_data) | (0 << 22) | (0 << 15);

        uint32_t tx_fctrl = 0;
        tx_fctrl |= (6 & 0x7F); // TFLEN
        tx_fctrl |= (0 << 7);   // TFLE = 0
        tx_fctrl |= (0 << 10);  // Reserved = 0
        tx_fctrl |= (2 << 13);  // TXBR = 10 (6.8 Mbps)
        tx_fctrl |= (1 << 16);  // TXPRF = 01 (16 MHz)
        tx_fctrl |= (1 << 18);  // TXPSR = 01
        tx_fctrl |= (1 << 20);  // PE = 01
        tx_fctrl |= (0 << 22);  // TXBOFFS = 0x000

        // dw1000_write_u32(TX_FCTRL, tx_fctrl);

        tx_start();
        // SYSTEM EVENT = 0x0F -> wait for transmission completion
        uint32_t sys_status;
        do
        {
            dw1000_read_u32(SYS_STATUS, &sys_status);
            k_msleep(10);
        } while (!(sys_status & SYS_STATUS_TXFRS)); // Check TXFRS bit

        LOG_INF("Transmission complete! Clearing bits in SYS_STATUS.");

        // SYSTEM EVENT = 0x0F -> clear TXFRS flag
        dw1000_write_u32(SYS_STATUS, SYS_STATUS_TXFRS);

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}