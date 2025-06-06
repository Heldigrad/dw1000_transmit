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

    set_rx_after_tx_delay(POLL_TX_TO_RESP_RX_DLY_UUS);
    // set_rx_timeout(RESP_RX_TIMEOUT_UUS * 500000);

    uint64_t T1, T2, T3, T4, aux;
    double values[20];
    double distance, sum;
    int cnt = 0;
    dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

    double mean = 0;

    for (int i = 0; i < 10; ++i)
    {
        T1 = 0;
        T2 = 0;
        T3 = 0;
        T4 = 0;
        dw1000_subwrite_u40(TX_TIME, 0x00, 0x00);
        dw1000_subwrite_u40(RX_TIME, 0x00, 0x00);

        transmit(POLL_MSG, 5, &T1);
        receive(&T2, &T4);
        transmit(0xA987654321, 5, &aux); // confirmation
        receive(&T3, &aux);

        distance = compute_distance(T1, T2, T3, T4);
        if (distance > 100)
        {
            continue;
        }
        LOG_INF("T1 = %llX, T2 = %llX, T3 = %llX, T4 = %llX, Distance = %f m", T1, T2, T3, T4, distance);
        sum += distance;
        cnt++;
        // k_msleep(RX_SLEEP_TIME_MS);

        // LOG_INF("T1 = %llX, T2 = %llX, T3 = %llX, T4 = %llX", T1, T2, T3, T4);
    }

    if (sum > 0)
    {
        sum /= cnt;
        LOG_INF("Raw mean = %f", sum);
    }
    return 0;
}