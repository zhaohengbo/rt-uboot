#ifndef GPIO_H
#define GPIO_H

enum gpio_idx_e {
	RST_BTN = 0,
	WPS_BTN,
	USB_BTN,
	USB3_BTN,
	PWR_LED,
	PWR_RED_LED,
	WPS_LED,
	WIFI_2G_LED,
	WIFI_5G_LED,
	USB_LED,
	USB3_LED,
	SATA_LED,
	WAN_LED,	/* WHITE or BLUE, depends on platform. */
	WAN2_LED,	/* WHITE or BLUE, depends on platform. */
	WAN_RED_LED,
	WAN2_RED_LED,
#if defined(CONFIG_HAVE_MULTI_LAN_LED)
	LAN1_LED,
	LAN2_LED,
	LAN3_LED,
	LAN4_LED,
#else
	LAN_LED,
#endif
	FAIL_OVER_LED,

	USB3_POWER,
#if defined(RT4GAC53U)
	LTE_RED_LED,
#endif
#if defined(CONFIG_HAVE_SOFT_SPI)
	SPI_SCLK,
	SPI_MOSI,
	SPI_CS,
#endif

	GPIO_IDX_MAX,	/* Last item */
};

extern void led_init(void);
extern void gpio_init(void);
extern void led_onoff(enum gpio_idx_e gpio_idx, int onoff);
extern void power_led_on(void);
extern void power_led_off(void);
extern void leds_on(void);
extern void leds_off(void);
extern void all_leds_on(void);
extern void all_leds_off(void);
extern unsigned long DETECT(void);
extern unsigned long DETECT_WPS(void);

#if defined(ALL_LED_OFF)
extern void enable_all_leds(void);
extern void disable_all_leds(void);
#else
static inline void enable_all_leds(void) { }
static inline void disable_all_leds(void) { }
#endif

#if defined(CONFIG_HAVE_WAN_RED_LED)
static inline void wan_red_led_on(void)
{
	led_onoff(WAN_RED_LED, 1);
	led_onoff(WAN2_RED_LED, 1);
}

static inline void wan_red_led_off(void)
{
	led_onoff(WAN_RED_LED, 0);
	led_onoff(WAN2_RED_LED, 0);
}
#else
static inline void wan_red_led_on(void) { }
static inline void wan_red_led_off(void) { }
#endif

#endif
