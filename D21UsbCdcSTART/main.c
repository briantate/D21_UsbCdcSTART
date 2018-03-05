#include <atmel_start.h>
#include "usb_start.h"
#define LED_PIN 59

/** Buffers to receive and echo the communication bytes. */
static uint32_t usbd_cdc_buffer[CONF_USB_CDCD_ACM_DATA_BULKIN_MAXPKSZ / 4];
//a string to print to the terminal
uint8_t hello_data[13] = {'h','e','l','l','o',' ','w','o','r','l','d',0xD, 0xA};
	
typedef enum 
{	
	DETACHED = 0,
	ATTACHED 
}ATTACHED_STATE_t;

/** 
 * \brief Function to initialize GPIO (non-START)
 */
static void init_gpio(void)
{
	//init gpio for Vbus detect PA14
	PORT->Group[0].DIRCLR.reg |= 0x1<<14; // PA14 is input so clear DIR bit	
}

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
	// wait for Data Terminal Ready before setting up endpoint callbacks
	if (state.rs232.DTR) { 
		
		// Callbacks must be registered after endpoint allocation
		cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)usb_device_cb_bulk_out);
		cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)usb_device_cb_bulk_in);
		
		//
		// start reading from simulated serial port, when it’s done the         
		// callback registered by cdcdf_acm_register_callback of type           
		// CDCDF_ACM_CB_READ will be invoked                                    
		//
		cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer));
		
		//send a string to the terminal
		cdcdf_acm_write(hello_data, 13);
		
	}

	/* No error. */
	return false;
}

int main(void)
{
	volatile ATTACHED_STATE_t attachedState = DETACHED;
	
	/************************************************************************/
	/* atmel_start_init() Initializes MCU, drivers and middleware           */
	/* From a USB perspective, this includes initialing USB stack and CDC   */
	/* ACM function driver, starting the USB device driver and attaching    */
	/* the USB device to the USB host                                       */
	/************************************************************************/ 
	atmel_start_init();
	
	init_gpio(); //init the VBus detect pin - this will later be handled by atmel_start init
	attachedState = ATTACHED;
	
	/************************************************************************/
	/* wait for acm function to be enabled                                  */
	/************************************************************************/
	while (!cdcdf_acm_is_enabled()) {
		// wait cdc acm to be installed
	};


	/************************************************************************/
	/* register callback for read/write/line coding change/state change.    */
	/* E.g., to monitor serial port open/close the state change callback    */
	/* should be registered.                                                */
	/************************************************************************/
	cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C, (FUNC_PTR)usb_device_cb_state_c); //don't care about DTR in our case
//	cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)usb_device_cb_bulk_out);   //Callback for data received from host
//	cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)usb_device_cb_bulk_in);   //Callback for data sent to host
	
	/************************************************************************/
	/* start reading from simulated serial port, when it’s done the         */
	/* callback registered by cdcdf_acm_register_callback of type           */
	/* CDCDF_ACM_CB_READ will be invoked                                    */
	/************************************************************************/
//	cdcdf_acm_read((uint8_t *)usbd_cdc_buffer, sizeof(usbd_cdc_buffer)); //wait for data from host

	//need to wait for host to be ready for data-- how to check this
	//if we don't wait, the data may go out but not seen by host?
//	delay_ms(1000);

	// send a string to the terminal
//	cdcdf_acm_write(hello_data, 13); 
		
	//turn on the LED
	PORT->Group[1].OUTCLR.reg |= (0x1<<30);
	
	while (1) {
//		gpio_toggle_pin_level(LED_PIN);
//		PORT->Group[1].OUTTGL.reg |= (0x1<<30);
//		delay_ms(500);	
		
		if((PORT->Group[0].IN.reg)||0x1<<14) //check if PA14 is set --> host is attached
		{
			switch(attachedState)
			{
				case ATTACHED:
				{
					//do nothing
					__asm("nop");
					__asm("nop");
					break;
				}
				case DETACHED: //host was previously detached
				{
					usbdc_attach();
					attachedState = ATTACHED;
					__asm("nop");
					break;
				}
				default:
				{
					//error --> do something
					break;
				}
			}
		}
		else //PA14 not set --> host detached
		{
			switch(attachedState)
			{
				case ATTACHED: //host was attached previously
				{
					//do nothing
					usbdc_detach();
					attachedState = DETACHED;
					__asm("nop");
					break;
				}
				case DETACHED:
				{
					//do nothing
					__asm("nop");
					__asm("nop");
					__asm("nop");
					break;
				}
				default:
				{
					//error --> do something
					break;
				}
			}
		}				
	}
}
