//*********************************************/
// TX
//*********************************************/

// #include "C:\Users\agape\Documents\LICENTA\functions\devices.h"
// #include "C:\Users\agape\Documents\LICENTA\functions\dw1000_ranging_functions.h"

#include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\devices.h"
#include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\dw1000_ranging_functions.h"

double distances[NR_OF_ANCHORS] = {0};

int main(void)
{
    if (check_devices_ready())
    {
        LOG_ERR("Devices not ready!");
        return 1;
    }
    gpio_pin_configure_dt(&reset_gpio, GPIO_OPEN_DRAIN | GPIO_OUTPUT);
    reset_devices();

    LOG_INF("INIT");

    uint8_t Dev_id = TAG_ID;

    bip_init();
    bip_config();

    uint64_t T1, T4;
    uint8_t Msg_id;
    double distance;

    while (1)
    {
        for (uint8_t anchor_id = 1; anchor_id <= NR_OF_ANCHORS; ++anchor_id)
        {
            set_antenna_delay(anchor_id);

            Msg_id = 0;
            distance = 0;

            while (1)
            {
                dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

                T1 = send_poll_message(Dev_id, anchor_id, Msg_id);
                if (ERR_LOGS_EN)
                {
                    LOG_INF("POLL %0d sent to anchor nr. %0d.", Msg_id, anchor_id);
                }

                dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);
                T4 = 0;

                if (get_resp_message(Dev_id, anchor_id, Msg_id, &T4, &distance) == SUCCESS)
                {
                    if (INFO_LOGS_EN)
                    {
                        LOG_INF("Response %0d received from anchor nr. %0d.", Msg_id, anchor_id);
                    }

                    if (distance != 0)
                    {
                        distances[anchor_id - 1] = distance;
                        LOG_INF("Distance from anchor %0d is %0.2fm.", anchor_id, distances[anchor_id - 1]);
                        break;
                    }
                    else
                    {
                        send_timestamps(Dev_id, anchor_id, T1, T4, Msg_id);
                        // LOG_INF("For msg = %0d, T1 = %0llX, T4 = %0llX", Msg_id, T1, T4);

                        if (INFO_LOGS_EN)
                        {
                            LOG_INF("Sent TS %0d to anchor nr. %0d.", Msg_id, anchor_id);
                        }
                    }
                }

                Msg_id++;
            }

            k_msleep(10);
        }
        LOG_INF("All distances acquired!");
    }
}