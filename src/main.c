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

    uint64_t T1, T2, T3, T4, aux;
    uint64_t Tround, Treply, Tprop;
    const double SPEED_OF_LIGHT = 299702547.0;
    double distance, tof;
    int ok = 1;
    dw1000_write_u32(SYS_STATUS, 0xFFFFFFFF);

    dw1000_subwrite_u40(TX_TIME, 0x00, 0x00);
    dw1000_subwrite_u40(RX_TIME, 0x00, 0x00);

    // while (1)
    {
        LOG_INF("\n\n");

        if (transmit(POLL_MSG, 5, &T1) == SUCCESS)
        {
            LOG_INF("TX Success! Waiting for timestamp nr. 2");
            if (receive(&T2, &T4) == SUCCESS)
            {
                LOG_INF("TX Success! Waiting for timestamp nr. 3");
                if (receive(&T3, &aux) == SUCCESS)
                {
                    LOG_INF("All required timestamps were acquired!");
                }
                else
                {
                    ok = 0;
                }
            }
            else
            {
                ok = 0;
            }
        }
        else
        {
            ok = 0;
        }

        if (ok == 0)
        {
            LOG_INF("Something went wrong. Please try again!");
        }
        else
        {
            Tround = T4 - T1;
            Tprop = (Tround - Treply) / 2;
            tof = (double)Tprop * DWT_TIME_UNITS;
            distance = Tprop * SPEED_OF_LIGHT;
            LOG_INF("T1 = %llX, T2 = %llX, T3 = %llX, T4 = %llX, Distance = %llX", T1, T2, T3, T4, distance);
        }

        k_msleep(RX_SLEEP_TIME_MS);
    }

    return 0;
}

// To try: 5.4 Transmit and automatically wait for response