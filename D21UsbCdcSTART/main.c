#include <atmel_start.h>
#include "usb_start.h"
#define LED_PIN 59

/** Buffers to receive and echo the communication bytes. */
static uint32_t usbd_cdc_buffer[CONF_USB_CDCD_ACM_DATA_BULKIN_MAXPKSZ / 4];

/**
 * \brief Callback invoked when bulk OUT data received
 */
static bool usb_device_cb_bulk_out(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	uint8_t test_data[9] = {'d','a','t','a',' ','r','x',0xD, 0xA};
	cdcdf_acm_write(test_data, 9);
	
	/* No error. */
	return false;
}

/**
 * \brief Callback invoked when bulk IN data received
 */
static bool usb_device_cb_bulk_in(const uint8_t ep, const enum usb_xfer_code rc, const uint32_t count)
{
	//prepare for next transfer from the host
	cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer));

	/* No error. */
	return false;
}

/**
 * \brief Callback invoked when Line State Change
 */
static bool usb_device_cb_state_c(usb_cdc_control_signal_t state)
{
	if (state.rs232.DTR) {
		/* Callbacks must be registered after endpoint allocation */
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)usb_device_cb_bulk_out);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)usb_device_cb_bulk_in);
		/* Start Rx */
		cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer));
	}

	/* No error. */
	return false;
}

int main(void)
{
	uint8_t hello_data[13] = {'h','e','l','l','o',' ','w','o','r','l','d',0xD, 0xA};
	
	/* Initializes MCU, drivers and middleware (including USB) */
	atmel_start_init(); //usb_init() called inside
	
	while (!cdcdf_acm_is_enabled()) {
		// wait cdc acm to be installed
	};


//	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)usb_device_cb_state_c); //don't care about DTR in our case
	cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)usb_device_cb_bulk_out);   //Callback for data received from host
	cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)usb_device_cb_bulk_in);   //Callback for data sent to host
	/* Start Rx */
	cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer)); //wait for data from host

	//need to wait for attaching to host -- how to check this
	delay_ms(1000);

	cdcdf_acm_write(hello_data, 13); 
		
	while (1) {
//		gpio_toggle_pin_level(LED_PIN);
		PORT->Group[1].OUTTGL.reg |= (0x1<<30);
		delay_ms(500);					
	}
}
