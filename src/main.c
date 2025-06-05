//*********************************************/
// TX
//*********************************************/

// #include "C:\Users\agape\Documents\LICENTA\functions\devices.h"
// #include "C:\Users\agape\Documents\LICENTA\functions\dw1000_ranging_functions.h"

#include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\devices.h"
#include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\dw1000_ranging_functions.h"

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

    dw1000_subwrite_u16(0x0C, 0x00, 0x4C33); // rx timeout

    set_rx_antenna_delay(RX_ANT_DLY);
    set_tx_antenna_delay(TX_ANT_DLY);

    set_rx_after_tx_delay(POLL_TX_TO_RESP_RX_DLY_UUS);
    set_rx_timeout(RESP_RX_TIMEOUT_UUS);

    uint64_t T1, T2, T3, T4, aux, status_reg;
    double distance;
    int cnt = 0;
    dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

    double mean = 0;

    for (int i = 0; i < 5; ++i)
    {
        LOG_INF("\n\n");

        dw1000_subwrite_u40(TX_TIME, 0x00, 0x00);
        dw1000_subwrite_u40(RX_TIME, 0x00, 0x00);

        // transmit poll message, then immediately wait for response
        dw1000_subwrite_u64(TX_BUFFER, 0x00, POLL_MSG);

        new_set_txfctrl(5);

        new_tx_start(2);

        if (!(status_reg & SYS_STATUS_ALL_RX_ERR))
        {
            T4 = get_rx_timestamp();
            T1 = get_tx_timestamp();
            dw1000_subread_u64(RX_BUFFER, 0x00, &T2);
            /* Clear good RX frame event in the DW1000 status register. */
            dw1000_write_u32(SYS_STATUS, SYS_STATUS_RXFCG);
        }
        else
        {
            LOG_INF("Errors encountered!");
            print_enabled_bits(status_reg);

            /* Clear RX error events in the DW1000 status register. */
            dw1000_write_u32(SYS_STATUS, SYS_STATUS_ALL_RX_ERR);
            rx_soft_reset();
        }

        receive(&T3, &aux);

        dw1000_write_u32(SYS_STATUS, SYS_STATUS_TX_OK); // Clear tx status ok

        distance = compute_distance(T1, T2, T3, T4);
        LOG_INF("T1 = %llX, T2 = %llX, T3 = %llX, T4 = %llX, Distance = %f m", T1, T2, T3, T4, distance);
        mean += distance;
        cnt++;
    }

    mean /= cnt;
    LOG_INF("Mean distance = %f m", mean);

    return 0;
}