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

    LOG_INF("INIT");

    uint8_t Dev_id = TAG_ID;

    bip_init();
    bip_config();

    set_rx_antenna_delay(RX_ANT_DLY);
    set_tx_antenna_delay(TX_ANT_DLY);

    uint64_t T1, T4;
    uint8_t Msg_id = 0;
    double distance;

    while (1)
    {
        for (uint8_t anchor_id = 1; anchor_id < 5; ++anchor_id)
        {
            while (1)
            {
                dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

                T1 = send_poll_message(Dev_id, anchor_id, Msg_id);
                if (ERR_LOGS_EN)
                {
                    LOG_INF("POLL sent to RESP.");
                }

                T4 = 0;
                if (get_resp_message(Dev_id, anchor_id, Msg_id, &T4, &distance) == SUCCESS)
                {
                    if (INFO_LOGS_EN)
                    {
                        LOG_INF("Response received from RESP.");
                    }

                    if (distance != 0)
                    {
                        LOG_INF("Distance from anchor %0d is %0.2fm", anchor_id, distance);
                        break;
                    }
                    else
                    {
                        send_timestamps(Dev_id, anchor_id, T1, T4, Msg_id);
                        // LOG_INF("For msg = %0d, T1 = %0llX, T4 = %0llX", Msg_id, T1, T4);

                        if (INFO_LOGS_EN)
                        {
                            LOG_INF("Sent TS %0d to RESP.", Msg_id);
                        }
                    }
                }

                Msg_id++;
            }
        }
    }
}