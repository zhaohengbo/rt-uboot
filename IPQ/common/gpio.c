/******************************************************************************
* Filename : gpio.c
* This part is used to control LED and detect button-press
******************************************************************************/

#include <common.h>
#include <command.h>
#include <gpio.h>
#include <asm/io.h>
#include <asm/arch-ipq40xx/iomap.h>
#include <asm/arch-qcom-common/gpio.h>
#include "../board/qcom/ipq40xx_cdp/ipq40xx_cdp.h"

#define INVALID_GPIO_NR	0xFFFFFFFF
#define GPIO_OE_ENABLE		1
#define GPIO_OE_DISABLE		0
#define GPIO_OUT_BIT		1
#define GPIO_IN_BIT		0

#define LED_ON 1
#define LED_OFF 0

/* LED/BTN definitions */
static struct gpio_s {
	char		*name;
	unsigned int	gpio_nr;	/* GPIO# */
	unsigned int	dir;		/* direction. 0: output; 1: input */
	unsigned int	is_led;		/* 0: NOT LED; 1: is LED */
	unsigned int	def_onoff;	/* default value of LEDs */
	unsigned int	active_low;	/* low active if non-zero */
	gpio_func_data_t data;
} gpio_tbl[GPIO_IDX_MAX] = {
#if defined(RTAC58U)
	[RST_BTN] = {	/* GPIO4, Low  active, input  */
		.name = "Reset button",
		.gpio_nr = 4,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[WPS_BTN] = {	/* GPIO63, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 63,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[PWR_LED] = {	/* GPIO3, High active, output */
		.name = "Power LED",
		.gpio_nr = 3,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_ON,
		.active_low = 0,
	},
	[WAN_LED] = {		/* GPIO1,  High active, output */
		.name = "WAN LED",
		.gpio_nr = 1,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[LAN_LED] = {		/* GPIO2,  High active, output */
		.name = "LAN LED",
		.gpio_nr = 2,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[WIFI_2G_LED] = {	/* GPIO58,  High active, output */
		.name = "2G LED",
		.gpio_nr = 58,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[WIFI_5G_LED] = {	/* GPIO5, High active, output */
		.name = "5G LED",
		.gpio_nr = 5,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[USB_LED] = {	/* GPIO0, High active, output */
		.name = "USB LED",
		.gpio_nr = 0,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
#elif defined(RTAC82U)
	[RST_BTN] = {
		.name = "Reset button",
		.gpio_nr = 18,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[WPS_BTN] = {
		.name = "WPS button",
		.gpio_nr = 11,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[PWR_LED] = {
		.name = "Power LED",
		.gpio_nr = 40,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_ON,
		.active_low = 1,
	},
	[WAN_LED] = {
		.name = "WAN LED",
		.gpio_nr = 61,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[WAN_RED_LED] = {
		.name = "WAN RED LED",
		.gpio_nr = 68,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[LAN1_LED] = {
		.name = "LAN1 LED",
		.gpio_nr = 49,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 1,
	},
	[LAN2_LED] = {
		.name = "LAN2 LED",
		.gpio_nr = 42,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 1,
	},
	[LAN3_LED] = {
		.name = "LAN3 LED",
		.gpio_nr = 43,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 1,
	},
	[LAN4_LED] = {
		.name = "LAN4 LED",
		.gpio_nr = 45,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 1,
	},
	[WIFI_2G_LED] = {
		.name = "2G LED",
		.gpio_nr = 52,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 1,
	},
	[WIFI_5G_LED] = {
		.name = "5G LED",
		.gpio_nr = 54,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 1,
	},
#elif defined(RT4GAC53U)
	[RST_BTN] = {	/* GPIO63, Low  active, input  */
		.name = "Reset button",
		.gpio_nr = 63,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[WPS_BTN] = {	/* GPIO5, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 5,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[LTE_RED_LED] = {	/* GPIO3, High active, output */
		.name = "LTE RED LED",
		.gpio_nr = 3,
		.dir = 0,
		.is_led = 1,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
#if defined(CONFIG_HAVE_SOFT_SPI)
	[SPI_SCLK] = {	/* GPIO58, High active, output */
		.name = "soft-SPI SCKL",	/* 74LVC595A SHCP */
		.gpio_nr = 58,
		.dir = 0,
		.is_led = 0,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[SPI_MOSI] = {	/* GPIO2, High active, output */
		.name = "soft-SPI MOSI",	/* 74LVC595A DS */
		.gpio_nr = 2,
		.dir = 0,
		.is_led = 0,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
	[SPI_CS] = {	/* GPIO4, High active, output */
		.name = "soft-SPI CS",		/* 74LVC595A STCP */
		.gpio_nr = 4,
		.dir = 0,
		.is_led = 0,
		.def_onoff = LED_OFF,
		.active_low = 0,
	},
#endif
#else
#error Define GPIO table!!!
#endif
};

/* Get real GPIO# of gpio_idx
 * @return:
 *  NULL:	GPIO# not found
 *  otherwise:	pointer to GPIO PIN's data
 */
static struct gpio_s *get_gpio_def(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g;

	if (gpio_idx < 0 || gpio_idx >= GPIO_IDX_MAX) {
		printf("%s: Invalid GPIO index %d/%d\n", __func__, gpio_idx, GPIO_IDX_MAX);
		return NULL;
	}

	g = &gpio_tbl[gpio_idx];
	if (!g->name)
		return NULL;
	return g;
}

/* Whether gpio_idx is active low or not
 * @return:	1:	active low
 * 		0:	active high
 */
static unsigned int get_gpio_active_low(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g)
		return INVALID_GPIO_NR;

	return !!(g->active_low);
}

/* Set GPIO# as GPIO PIN and direction.
 * @gpio_nr:	GPIO#
 * @dir:	GPIO direction
 * 		0: output
 * 		1: input.
 */
static void __ipq40xx_set_gpio_dir(enum gpio_idx_e gpio_idx, int dir)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);
	gpio_func_data_t data = {
		.func = 0,		/* GPIO_IN_OUT */
		.pull = GPIO_NO_PULL,
		.drvstr = GPIO_8MA,
	};

	if (!g)
		return;

	data.gpio = g->gpio_nr;
	if (!dir) {
		data.out = GPIO_OUTPUT;
		data.oe = GPIO_OE_ENABLE;
	} else {
		data.out = GPIO_INPUT;
		data.oe = GPIO_OE_DISABLE;
	}
	qca_configure_gpio(&data, 1);
}

/* Set raw value to GPIO#
 * @gpio_nr:	GPIO#
 * @val:	GPIO direction
 * 		0: low-level voltage
 * 		1: high-level voltage
 */
static void __ipq40xx_set_gpio_pin(enum gpio_idx_e gpio_idx, int val)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);
	uint32_t addr, reg, mask;

	if (!g)
		return;

	addr = GPIO_IN_OUT_ADDR(g->gpio_nr);
	reg = readl(addr);

	mask = 1U << GPIO_OUT_BIT;
	if (!val) {
		/* output 0 */
		reg &= ~mask;
	} else {
		/* output 1 */
		reg |= mask;
	}

	writel(reg, addr);
}

/* Read raw value of GPIO#
 * @gpio_nr:	GPIO#
 * @return:
 * 		0: low-level voltage
 * 		1: high-level voltage
 */
static int __ipq40xx_get_gpio_pin(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);
	uint32_t addr, reg;

	if (!g)
		return 0;

	addr = GPIO_IN_OUT_ADDR(g->gpio_nr);
	reg = readl(addr) && (1 << GPIO_IN_BIT);

	return !!reg;
}

/* Check button status. (high/low active is handled in this function)
 * @return:	1: key is pressed
 * 		0: key is not pressed
 */
static int check_button(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g)
		return 0;

	return !!(__ipq40xx_get_gpio_pin(gpio_idx) ^ get_gpio_active_low(gpio_idx));
}

/* Check button status. (high/low active is handled in this function)
 * @onoff:	1: Turn on LED
 * 		0: Turn off LED
 */
void led_onoff(enum gpio_idx_e gpio_idx, int onoff)
{
	__ipq40xx_set_gpio_pin(gpio_idx, onoff ^ get_gpio_active_low(gpio_idx));
}

#if defined(CONFIG_HAVE_SOFT_SPI)
/*
 * Only support SPI mode 0 (CPOL = 0, CPHA = 0)
 */
#define SOFT_SPI_SCL(onoff)	led_onoff(SPI_SCLK, onoff)
#define SOFT_SPI_SDA(onoff)	led_onoff(SPI_MOSI, onoff)
#define SOFT_SPI_CS(onoff)	led_onoff(SPI_CS, onoff)
#define SOFT_SPI_DELAY		udelay(5)

/*
 * CPU control LED by software SPI control shift register (e.g. 74LVC595A)
 * @dout:	0x0 ~ 0xffff:	on/off
 * @bitlen:	8/16:		out length
 */
void soft_spi_xfer(uint16_t dout, uint8_t bitlen)
{
	uint16_t tmpdout = dout;
	uint8_t i;

	SOFT_SPI_CS(0);
	SOFT_SPI_SCL(0);
	for (i = 0; i < bitlen; i++) {
		SOFT_SPI_SDA((tmpdout & 0x8000) ? 1 : 0);
		SOFT_SPI_DELAY;
		SOFT_SPI_SCL(1);
		tmpdout <<= 1;
		SOFT_SPI_DELAY;
		SOFT_SPI_SCL(0);
	}
	SOFT_SPI_CS(1);
}
#endif

void led_init(void)
{
	int i, on;

	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || gpio_tbl[i].dir)
			continue;
		__ipq40xx_set_gpio_dir(i, 0);
		on = 1;
		if (i == WAN_RED_LED || i == WAN2_RED_LED)
			on = 0;
		led_onoff(i, on);	/* turn on all LEDs, except WANx RED LED */
	}
	udelay(300 * 1000UL);
	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || gpio_tbl[i].dir)
			continue;
		__ipq40xx_set_gpio_dir(i, 0);
		led_onoff(i, gpio_tbl[i].def_onoff);
	}

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0xfffe, 16);
#endif
#endif
}

void gpio_init(void)
{
	printf("ASUS %s gpio init : wps / reset pin\n", model);
	__ipq40xx_set_gpio_dir(WPS_BTN, 1);
	__ipq40xx_set_gpio_dir(RST_BTN, 1);
}

unsigned long DETECT(void)
{
	int key = 0;

	if (check_button(RST_BTN)) {
		key = 1;
		printf("reset buootn pressed!\n");
	}

	return key;
}

unsigned long DETECT_WPS(void)
{
	int key = 0;

	if (check_button(WPS_BTN)) {
		key = 1;
		printf("wps buootn pressed!\n");
	}

	return key;
}

void power_led_on(void)
{
	led_onoff(PWR_LED, 1);

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0xfffe, 16);
#endif
#endif
}

void power_led_off(void)
{
	led_onoff(PWR_LED, 0);

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0xffff, 16);
#endif
#endif
}

/* Turn on model-specific LEDs */
void leds_on(void)
{
	led_onoff(PWR_LED, 1);
	led_onoff(WAN_LED, 1);
	led_onoff(WAN_RED_LED, 1);
	led_onoff(WAN2_LED, 1);
#if defined(CONFIG_HAVE_MULTI_LAN_LED)
	led_onoff(LAN1_LED, 1);
	led_onoff(LAN2_LED, 1);
	led_onoff(LAN3_LED, 1);
	led_onoff(LAN4_LED, 1);
#else
	led_onoff(LAN_LED, 1);
#endif
	led_onoff(WIFI_2G_LED, 1);
	led_onoff(WIFI_5G_LED, 1);

	/* Don't turn on below LEDs in accordance with PM's request. */
	led_onoff(PWR_RED_LED, 0);
	wan_red_led_off();
	led_onoff(USB_LED, 0);
	led_onoff(USB3_LED, 0);
	led_onoff(WPS_LED, 0);
	led_onoff(FAIL_OVER_LED, 0);

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0x0fc, 16);
#endif
#endif
}

/* Turn off model-specific LEDs */
void leds_off(void)
{
	led_onoff(PWR_LED, 0);
	led_onoff(WAN_LED, 0);
	led_onoff(WAN_RED_LED, 0);
	led_onoff(WAN2_LED, 0);
#if defined(CONFIG_HAVE_MULTI_LAN_LED)
	led_onoff(LAN1_LED, 0);
	led_onoff(LAN2_LED, 0);
	led_onoff(LAN3_LED, 0);
	led_onoff(LAN4_LED, 0);
#else
	led_onoff(LAN_LED, 0);
#endif
	wan_red_led_off();

	led_onoff(PWR_RED_LED, 0);
	led_onoff(USB_LED, 0);
	led_onoff(USB3_LED, 0);
	led_onoff(WIFI_2G_LED, 0);
	led_onoff(WIFI_5G_LED, 0);
	led_onoff(WPS_LED, 0);
	led_onoff(FAIL_OVER_LED, 0);

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0xffff, 16);
#endif
#endif
}

/* Turn on all model-specific LEDs */
void all_leds_on(void)
{
	int i;

	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || !gpio_tbl[i].is_led)
			continue;
		led_onoff(i, 1);
	}

	/* WAN RED LED share same position with WAN BLUE LED. Turn on WAN BLUE LED only*/
	wan_red_led_off();

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0x0fc, 16);
#endif
#endif
}

/* Turn off all model-specific LEDs */
void all_leds_off(void)
{
	int i;

	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || !gpio_tbl[i].is_led)
			continue;
		led_onoff(i, 0);
	}

	wan_red_led_off();

#if defined(CONFIG_HAVE_SOFT_SPI)
#if defined(RT4GAC53U)
	soft_spi_xfer(0xffff, 16);
#endif
#endif
}

#if defined(ALL_LED_OFF)
void enable_all_leds(void)
{
}

void disable_all_leds(void)
{
}
#endif

#if defined(DEBUG_LED_GPIO)
int do_test_gpio (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	struct gpio_s *g = NULL;
	int i, j, stop, old = 0, new = 0, status;
	unsigned int gpio_idx = GPIO_IDX_MAX;

	if (argc >= 2) {
		gpio_idx = simple_strtoul(argv[1], 0, 10);
		g = get_gpio_def(gpio_idx);
	}
	if (!g) {
		printf("%8s %20s %5s %9s %10s \n", "gpio_idx", "name", "gpio#", "direction", "active low");
		for (i = 0; i < GPIO_IDX_MAX; ++i) {
			if (!(g = get_gpio_def(i)))
				continue;
			printf("%8d %20s %5d %9s %10s \n", i, g->name, g->gpio_nr,
				(!g->dir)?"output":"input", (g->active_low)? "yes":"no");
		}
		return 1;
	}

	printf("%s: GPIO index %d GPIO#%d direction %s active_low %s\n",
		g->name, gpio_idx, g->gpio_nr,
		(!g->dir)?"output":"input", (g->active_low)? "yes":"no");
	printf("Press any key to stop testing ...\n");
	if (!g->dir) {
		/* output */
		for (i = 0, stop = 0; !stop; ++i) {
			printf("%s: %s\n", g->name, (i&1)? "ON":"OFF");
			led_onoff(gpio_idx, i & 1);
			for (j = 0, stop = 0; !stop && j < 40; ++j) {
				udelay(100000);
				if (tstc())
					stop = 1;
			}
		}
	} else {
		/* input */
		for (i = 0, stop = 0; !stop; ++i) {
			new = __ipq40xx_get_gpio_pin(gpio_idx);
			status = check_button(gpio_idx);
			if (!i || old != new) {
				printf("%s: %d [%s]\n", g->name, new, status? "pressed":"not pressed");
				old = new;
			}
			for (j = 0, stop = 0; !stop && j < 10; ++j) {
				udelay(5000);
				if (tstc())
					stop = 1;
			}
		}
	}

	return 0;
}

U_BOOT_CMD(
    test_gpio, 2, 0, do_test_gpio,
    "test_gpio - Test GPIO.\n",
    "test_gpio [<gpio_idx>] - Test GPIO PIN.\n"
    "                <gpio_idx> is the index of GPIO table.\n"
    "                If gpio_idx is invalid or is not specified,\n"
    "                GPIO table is printed.\n"
);


int do_ledon(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	leds_on();

	return 0;
}

int do_ledoff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	leds_off();

	return 0;
}

U_BOOT_CMD(
    ledon, 1, 1, do_ledon,
	"ledon\t -set led on\n",
	NULL
);

U_BOOT_CMD(
    ledoff, 1, 1, do_ledoff,
	"ledoff\t -set led off\n",
	NULL
);

#if defined(ALL_LED_OFF)
int do_all_ledon(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enable_all_leds();

	return 0;
}

int do_all_ledoff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	disable_all_leds();

	return 0;
}

U_BOOT_CMD(
    all_ledon, 1, 1, do_all_ledon,
	"all_ledon\t -set all_led on\n",
	NULL
);

U_BOOT_CMD(
    all_ledoff, 1, 1, do_all_ledoff,
	"all_ledoff\t -set all_led off\n",
	NULL
);
#endif

#endif	/* DEBUG_LED_GPIO */
