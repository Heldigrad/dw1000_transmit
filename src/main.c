//*********************************************/
// TX
//*********************************************/

#include "C:\Users\agape\Documents\LICENTA\functions\devices.h"
#include "C:\Users\agape\Documents\LICENTA\functions\dw1000_ranging_functions.h"

// #include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\devices.h"
// #include "C:\Users\agape\Documents\LICENTA\dw1000_app\functions\dw1000_ranging_functions.h"

double distances[NR_OF_ANCHORS] = {0};

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

    uint64_t T1, T2, T3, T4, T5, T6;
    uint8_t Msg_id;
    double distance;
    int ret;

    while (1)
    {
        for (uint8_t anchor_id = 1; anchor_id <= NR_OF_ANCHORS; ++anchor_id)
        {
            set_antenna_delay(anchor_id);

            Msg_id = 0;
            distance = 0;

            dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

            T1 = send_poll1_message(Dev_id, anchor_id, Msg_id);
            do
            {
                ret = get_msg_from_resp(anchor_id, &T2, &T3, &T4, &T5, &T6, Msg_id);

                if (ret == FAILURE)
                {
                    dw1000_write_u32(SYS_STATUS, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_RX_OK);
                    rx_soft_reset();

                    T2 = 0;
                    T3 = 0;
                    T4 = 0;
                    T5 = 0;
                    T6 = 0;

                    Msg_id++;

                    k_msleep(10000);
                    T1 = send_poll1_message(Dev_id, anchor_id, Msg_id);
                }

            } while (T2 == 0 || T3 == 0 || T6 == 0);

            LOG_INF_IF_ENABLED("For ranging attempt nr. %0d:", Msg_id);
            LOG_INF_IF_ENABLED("T1 = %0llX, T4 = %0llX, T5 = %0llX", T1, T4, T5);
            LOG_INF_IF_ENABLED("T2 = %0llX, T3 = %0llX, T6 = %0llX", T2, T3, T6);

            uint64_t Treply1 = T3 - T2;
            uint64_t Treply2 = T5 - T4;
            uint64_t Tround1 = T4 - T1;
            uint64_t Tround2 = T6 - T3;

            LOG_INF_IF_ENABLED("Treply1 = %0llX, Treply2 = %0llX, Tround1 = %0llX, Tround2 = %0llX", Treply1, Treply2, Tround1, Tround2);

            k_msleep(10000);
        }
        LOG_INF_IF_ENABLED("All distances acquired!\n\r");
    }
}