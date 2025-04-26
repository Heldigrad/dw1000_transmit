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

    while (1)
    {
        LOG_INF("\n\n");
        LOG_INF("TX");

        // Device ID
        // uint32_t dev_id;
        // dw1000_read_u32(0x00, &dev_id);

        reset_devices();

        generic_default_configs(4);
        tx_default_configs();
        additional_default_configs();

        // TX_BUFFER = 0x09
        uint32_t tx_data = POLL_MSG; // Poll message - to initiate ranging
        dw1000_write_u32(TX_BUFFER, tx_data);

        tx_start();
        // SYSTEM EVENT = 0x0F -> wait for transmission completion
        uint32_t sys_status;
        do
        {
            dw1000_read_u32(SYS_STATUS, &sys_status);
            k_msleep(10);
        } while (!(sys_status & SYS_STATUS_TXFRS)); // Check TXFRS bit

        uint64_t T1 = get_tx_timestamp();
        LOG_INF("TX success! T1 = %llX", T1);

        // SYSTEM EVENT = 0x0F -> clear TXFRS flag
        dw1000_write_u32(SYS_STATUS, SYS_STATUS_TXFRS);

        // RECEIVE response
        // generic_default_configs(4);
        // rx_default_configs();
        // // additional_default_configs();

        // rx_enable();

        // // Wait for a valid frame (RXFCG bit in SYS_STATUS)
        // uint32_t sys_status_1 = {0};
        // do
        // {
        //     dw1000_read_u32(SYS_STATUS, &sys_status_1);
        //     k_msleep(100);
        // } while (!(sys_status_1 & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR))); // Check ERR bits | RXFCG bit

        // if (sys_status_1 & SYS_STATUS_RXFCG)
        // {
        //     // Read received data
        //     uint64_t T4 = get_rx_timestamp();
        //     LOG_INF("Reception success! T4 = %X", T4);
        //     uint32_t T2;
        //     dw1000_read_u32(RX_BUFFER, &T2);

        //     // Clear status bit
        //     // LOG_INF("Clearing status bit...");
        //     dw1000_write_u32(SYS_STATUS, SYS_STATUS_RX_OK);
        // }
        // else
        // {
        //     // Clear err bits
        //     LOG_INF("Reception failed. Resend message! Clearing err bits...");
        //     dw1000_write_u32(SYS_STATUS, SYS_STATUS_ALL_RX_ERR);
        // }

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}