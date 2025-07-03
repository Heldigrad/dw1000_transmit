//*********************************************/
// TX
//*********************************************/

#include "C:\Users\agape\Documents\LICENTA\functions\devices.h"
#include "C:\Users\agape\Documents\LICENTA\functions\dw1000_ranging_functions.h"

// #include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\devices.h"
// #include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\dw1000_ranging_functions.h"

double distances[NR_OF_ANCHORS] = {0};
float clockOffsetRatio;

int main(void)
{
    if (check_devices_ready())
    {
        LOG_ERR_IF_ENABLED("Devices not ready!");
        return 1;
    }
    gpio_pin_configure_dt(&reset_gpio, GPIO_OPEN_DRAIN | GPIO_OUTPUT);
    reset_devices();

    LOG_INF_IF_ENABLED("INIT");

    uint8_t Dev_id = TAG_ID;

    bip_init();
    bip_config();

    static uint64_t T1 = 0, T4 = 0, T6 = 0;
    static uint64_t T2 = 0, T3 = 0, T5 = 0;
    uint8_t Msg_id = 0;
    double distance;
    int ret;

    while (1)
    {
        for (uint8_t anchor_id = 1; anchor_id <= NR_OF_ANCHORS; ++anchor_id)
        {
            set_antenna_delay(anchor_id);

            distance = 0;

            dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

            Msg_id += 1000;

            T1 = send_poll1_message(Dev_id, anchor_id, Msg_id);
            do
            {
                ret = get_msg_from_resp(anchor_id, &T2, &T3, &T4, &T5, &T6, Msg_id);

                if (ret == FAILURE)
                {
                    LOG_ERR_IF_ENABLED("Something went wrong. Abandoning this ranging cycle.");

                    dw1000_write_u32(SYS_STATUS, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_RX_OK);
                    rx_soft_reset();

                    T2 = 0;
                    T3 = 0;
                    T4 = 0;
                    T5 = 0;
                    T6 = 0;

                    Msg_id += 1000;

                    k_msleep(500);

                    T1 = (uint32_t)send_poll1_message(Dev_id, anchor_id, Msg_id);
                }

            } while (T2 == 0 || T3 == 0 || T6 == 0);

            clockOffsetRatio = read_carrier_integrator() *
                               (FREQ_OFFSET_MULTIPLIER * HERTZ_TO_PPM_MULTIPLIER_CHAN_2 / 1.0e6);

            LOG_INF_IF_ENABLED("For ranging cycle nr. %0d:", Msg_id);
            LOG_INF_IF_ENABLED("T1 = %0llu, T4 = %0llu, T5 = %0llu", T1, T4, T5);
            LOG_INF_IF_ENABLED("T2 = %0llu, T3 = %0llu, T6 = %0llu", T2, T3, T6);

            int64_t Tround1 = T4 - T1;
            int64_t Tround2 = T6 - T3;

            distance = compute_ds_twr_distance_basic(T1, T2, T3, T4, T5, T6);
            distances[anchor_id - 1] = distance;

            LOG_WRN("Distance from board %0d is %0f", anchor_id, distance);

            k_msleep(500);
        }

        LOG_INF("All distances acquired!");
    }
}