/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <asm/byteorder.h>
#include "httpd.h"

#include "../httpd/uipopt.h"
#include "../httpd/uip.h"
#include "../httpd/uip_arp.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
extern void led_on(void);
extern void led_off(void);
//#include <gpio.h>
#include <spi_api.h>

static int arptimer = 0;

void HttpdHandler( void ){
	int i;

	if ( uip_len == 0 ) {
		for ( i = 0; i < UIP_CONNS; i++ ) {
			uip_periodic( i );
			if ( uip_len > 0 ) {
				uip_arp_out();
				NetSendHttpd();
			}
		}

		if ( ++arptimer == 20 ) {
			uip_arp_timer();
			arptimer = 0;
		}
	} else {
		if ( BUF->type == htons( UIP_ETHTYPE_IP ) ) {
			uip_arp_ipin();
			uip_input();
			if ( uip_len > 0 ) {
				uip_arp_out();
				NetSendHttpd();
			}
		} else if ( BUF->type == htons( UIP_ETHTYPE_ARP ) ) {
			uip_arp_arpin();
			if ( uip_len > 0 ) {
				NetSendHttpd();
			}
		}
	}

}

// start http daemon
void HttpdStart( void ){
	uip_init();
	httpd_init();
}

int do_http_upgrade( const ulong size, const int upgrade_type ){

	if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_UBOOT ) {

		printf( "\n\n****************************\n*     U-BOOT UPGRADING     *\n* DO NOT POWER OFF DEVICE! *\n****************************\n\n" );
		return( raspi_erase_write( ( u8_t * )WEBFAILSAFE_UPLOAD_RAM_ADDRESS, WEBFAILSAFE_UPLOAD_UBOOT_ADDRESS - CFG_FLASH_BASE, WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES ) );

	} else if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE ) {

		printf( "\n\n****************************\n*    FIRMWARE UPGRADING    *\n* DO NOT POWER OFF DEVICE! *\n****************************\n\n" );
		return( raspi_erase_write( ( u8_t * )WEBFAILSAFE_UPLOAD_RAM_ADDRESS, WEBFAILSAFE_UPLOAD_KERNEL_ADDRESS - CFG_FLASH_BASE, size ) );

	} else if ( upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_ART ) {

		printf( "\n\n****************************\n*      ART  UPGRADING      *\n* DO NOT POWER OFF DEVICE! *\n****************************\n\n" );
		return( raspi_erase_write( ( u8_t * )WEBFAILSAFE_UPLOAD_RAM_ADDRESS, WEBFAILSAFE_UPLOAD_ART_ADDRESS - CFG_FLASH_BASE, WEBFAILSAFE_UPLOAD_ART_SIZE_IN_BYTES ) );

	} else {
		return(-1);
	}

	return(-1);
}

// info about current progress of failsafe mode
int do_http_progress( const int state ){
	unsigned char i = 0;

	/* toggle LED's here */
	switch ( state ) {
		case WEBFAILSAFE_PROGRESS_START:

			// blink LED fast 10 times
			for ( i = 0; i < 10; ++i ) {
				led_on();
				udelay( 150000 );
				led_off();
				udelay( 150000 );
			}

			printf( "HTTP server is ready!\n\n" );
			break;

		case WEBFAILSAFE_PROGRESS_TIMEOUT:
			//printf("Waiting for request...\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPLOAD_READY:

			// blink LED fast 10 times
			for ( i = 0; i < 10; ++i ) {
				led_on();
				udelay( 25000 );
				led_off();
				udelay( 25000 );
			}
			printf( "HTTP upload is done! Upgrading...\n" );
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_READY:

			// blink LED fast 10 times
			for ( i = 0; i < 10; ++i ) {
				led_on();
				udelay( 150000 );
				led_off();
				udelay( 150000 );
			}
			printf( "HTTP ugrade is done! Rebooting...\n\n" );
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_FAILED:
			printf( "## Error: HTTP ugrade failed!\n\n" );

			// blink LED fast for 4 sec
			for ( i = 0; i < 80; ++i ) {
				led_on();
				udelay( 25000 );
				led_off();
				udelay( 25000 );
			}

			// wait 1 sec
			udelay( 1000000 );

			break;
	}

	return( 0 );
}
