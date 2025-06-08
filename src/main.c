//*********************************************/
// TX
//*********************************************/

#include "C:\Users\agape\Documents\LICENTA\functions\devices.h"
#include "C:\Users\agape\Documents\LICENTA\functions\dw1000_ranging_functions.h"

int main(void)
{
    if (check_devices_ready())
    {
        LOG_ERR("Devices not ready!");
        return 1;
    }
    gpio_pin_configure_dt(&reset_gpio, GPIO_OPEN_DRAIN | GPIO_OUTPUT);
    reset_devices();

    LOG_INF("TX");

    uint8_t Dev_id = 0x00;

    bip_init();
    bip_config();

    set_rx_antenna_delay(RX_ANT_DLY);
    set_tx_antenna_delay(TX_ANT_DLY);

    // set_rx_after_tx_delay(POLL_TX_TO_RESP_RX_DLY_UUS);
    // set_rx_timeout(RESP_RX_TIMEOUT_UUS * 8000000);

    uint64_t T1, T4;
    dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);
    int kk;
    uint8_t Msg_id = 0;

    while (1)
    {
        dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

        T1 = send_poll_message(Dev_id, 0x01);
        k_msleep(5);

        T4 = 0;
        if (get_resp_message(0x01, Dev_id, &T4) == SUCCESS)
        {
            k_msleep(5);
            send_timestamps(Dev_id, T1, T4);
        }
        k_msleep(5);
    }
}