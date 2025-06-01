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

    dw1000_subwrite_u16(0x0C, 0x00, 0x4C33); // rx timeout

    set_rx_antenna_delay(RX_ANT_DLY);
    set_tx_antenna_delay(TX_ANT_DLY);

    uint64_t T1, T2, T3, T4, aux;
    double distance;
    int cnt = 0;
    dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

    double mean = 0;

    for (int i = 0; i < 10; ++i)
    {
        LOG_INF("\n\n");

        dw1000_subwrite_u40(TX_TIME, 0x00, 0x00);
        dw1000_subwrite_u40(RX_TIME, 0x00, 0x00);

        if (transmit(POLL_MSG, 5, &T1) == SUCCESS)
        {
            // LOG_INF("TX Success! Waiting for timestamp nr. 2");
            if (receive(&T2, &T4) == SUCCESS)
            {
                // LOG_INF("TX Success! Waiting for timestamp nr. 3");
                if (receive(&T3, &aux) == SUCCESS)
                {
                    // LOG_INF("All required timestamps were acquired!");
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
        else
        {
            continue;
        }

        distance = compute_distance(T1, T2, T3, T4);
        LOG_INF("T1 = %llX, T2 = %llX, T3 = %llX, T4 = %llX, Distance = %f m", T1, T2, T3, T4, distance);
        mean += distance;
        cnt++;
        // k_msleep(RX_SLEEP_TIME_MS);
    }

    mean /= cnt;
    LOG_INF("Mean distance = %f m", mean);

    return 0;
}

// To try: 5.4 Transmit and automatically wait for response