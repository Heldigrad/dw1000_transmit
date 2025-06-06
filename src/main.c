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

    bip_init();
    bip_config();

    // dw1000_subwrite_u16(0x0C, 0x00, 0x4C33); // rx timeout

    set_rx_antenna_delay(RX_ANT_DLY);
    set_tx_antenna_delay(TX_ANT_DLY);

    // set_rx_after_tx_delay(POLL_TX_TO_RESP_RX_DLY_UUS);
    //  set_rx_timeout(RESP_RX_TIMEOUT_UUS * 500000);

    uint64_t T1, T4, aux;
    dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

    double mean = 0;

    while (1)
    {
        T1 = 0;
        T4 = 0;
        dw1000_subwrite_u40(TX_TIME, 0x00, 0x00);
        dw1000_subwrite_u40(RX_TIME, 0x00, 0x00);

        transmit(POLL_MSG, 5, &T1);
        receive(&aux, &T4);
        if (aux == 0x1020304050)
        {
            dw1000_subwrite_u40(TX_BUFFER, 0x00, T1);
            dw1000_subwrite_u40(TX_BUFFER, 0x05, T4);

            new_set_txfctrl(10);

            new_tx_start(0);

            uint32_t status;
            do
            {
                dw1000_read_u32(SYS_STATUS, &status);
            } while (!(status & SYS_STATUS_TXFRS | SYS_STATUS_ALL_TX_ERR));

            if (!(status & SYS_STATUS_ALL_TX_ERR))
            {
                /* Clear TX frame sent event. */
                dw1000_write_u32(SYS_STATUS, SYS_STATUS_TX_OK);
            }
            else
            {
                /* Clear TX error event. */
                dw1000_write_u32(SYS_STATUS, SYS_STATUS_TX_OK | SYS_STATUS_ALL_TX_ERR);
            }
        }
        LOG_INF("T1 = %llX, T4 = %llX", T1, T4);
    }
}