//*********************************************/
// TX
//*********************************************/

#include "C:\Users\agape\Documents\LICENTA\functions\includes.h"

int main(void)
{
    if(check_devices_ready()){
        LOG_ERR("Devices not ready!");
        return 1;
    }
    gpio_pin_configure_dt(&reset_gpio, GPIO_OPEN_DRAIN|GPIO_OUTPUT);
    reset_devices();
    LOG_INF("\n[TX]");
    uint32_t dev_id;
    dw1000_read_u32(0x00, &dev_id);
    initialize();
    // dw1000_default_config();

    //configure();

    while (1)
    {
        LOG_INF("SIMPLE_TX");

        // Device ID
        uint32_t dev_id;
        dw1000_read_u32(0x00, &dev_id);

        // TX_BUFFER = 0x09
        uint32_t tx_data = 0xACACACAC;             
        dw1000_write_u32(0x09, tx_data); 
        
        uint32_t reg32 = pdw1000local.txFCTRL | sizeof(tx_data) | (0 << 22) | (0 << 15);
        dw1000_write_u32(TX_FCTRL, reg32);
        
        tx_start();
        // SYSTEM EVENT = 0x0F -> wait for transmission completion
        uint32_t sys_status;
        do
        {
            dw1000_read_u32(SYS_STATUS, &sys_status);
            k_msleep(10);
        } while (!(sys_status & SYS_STATUS_TXFRS)); // Check TXFRS bit

        LOG_INF("Transmission complete! Clearing bits in SYS_STATUS.");

        // SYSTEM EVENT = 0x0F -> clear TXFRS flag
        dw1000_write_u32(SYS_STATUS, SYS_STATUS_TXFRS);

        k_msleep(SLEEP_TIME_MS);
    }
 
    return 0;
}