//*********************************************/
// TX
//*********************************************/

#include "C:\Users\agape\Documents\LICENTA\new_1\functions\spi_operations.h"
#include "C:\Users\agape\Documents\LICENTA\new_1\functions\defines.h"
#include "C:\Users\agape\Documents\LICENTA\new_1\functions\dw_functions.h"
#include "C:\Users\agape\Documents\LICENTA\new_1\functions\includes.h"

int main(void)
{
    check_dev_status();
    reset_device();

    // initialize();

    // configure();

    dw1000_restore_default();

    while (1)
    {
        LOG_INF("SIMPLE_TX");

        /*// Device ID
        uint8_t dev_id[4] = {0};
        dw1000_spi_read(spi_dev, 0x00, dev_id, sizeof(dev_id));
        */

        // TX_BUFFER = 0x09
        uint32_t tx_data = 0xDEDEDEDE;

        dw1000_spi_write_32(TX_BUFFER, tx_data);

        uint32_t tx_fctrl = 0;
        tx_fctrl |= 4;           // FLEN (frame length)
        tx_fctrl |= (0 << 10);   // Ranging disabled
        tx_fctrl |= (0x2 << 11); // Data rate = 6.8 Mbps
        tx_fctrl |= (0 << 13);   // TR = 0 (standard PHR)
        tx_fctrl |= (0x5 << 18); // TXPSR = 128-symbol preamble
        // No need to set PE bits for default

        dw1000_spi_write_32(TX_FCTRL, tx_fctrl);
        /*
        uint32_t reg32 = pdw1000local.txFCTRL | sizeof(tx_data) | (0 << 22) | (0 << 15);
        dw1000_spi_write_32(TX_FCTRL, reg32);
        */
        tx_start();
        // SYSTEM EVENT = 0x0F -> wait for transmission completion
        uint32_t sys_status;
        do
        {
            dw1000_spi_read(SYS_STATUS, sys_status);
            k_msleep(10);
        } while (!(sys_status & SYS_STATUS_TXFRS)); // Check TXFRS bit

        LOG_INF("Transmission complete! Clearing bits in SYS_STATUS.");

        // SYSTEM EVENT = 0x0F -> clear TXFRS flag
        dw1000_spi_write(SYS_STATUS, SYS_STATUS_TXFRS);
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}