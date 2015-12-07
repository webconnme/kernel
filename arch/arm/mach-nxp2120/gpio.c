#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/io.h>

#include <mach/platform.h>
#include <mach/gpio.h>
#include <mach/soc.h>

#define NEXELL_GPIO_BANK_NUM	6
#define GPIO_NUM_PER_BANK			32

//#define NEXELL_GPIO_DEBUG
#ifdef NEXELL_GPIO_DEBUG
#define DBG(x...) 	printk(x)
#else
#define DBG(x...)
#endif

struct gpio_bank {
	int index; /* Bank Index : A(0), B(1), C(2), D(3), E(4), ALIVE(5) */
	struct gpio_chip chip;
};

/* gpio_chip function
 * see asm-generic/gpio.h
 */
static int nexell_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static void nexell_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	return;
}

static int nexell_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned int gpio_num;
	struct gpio_bank *bank;

	bank = container_of(chip, struct gpio_bank, chip);
	gpio_num = bank->index * GPIO_NUM_PER_BANK + offset;
	soc_gpio_set_io_func(gpio_num, 0);	// GPIO PAD FUNC
	soc_gpio_set_io_dir(gpio_num, 0);

	DBG("%s: offset(%d), gpio_num(%d)\n", __func__, offset, gpio_num);
	return 0;
}

static int nexell_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	unsigned int gpio_num;
	struct gpio_bank *bank;

	bank = container_of(chip, struct gpio_bank, chip);
	gpio_num = bank->index * GPIO_NUM_PER_BANK + offset;
	DBG("%s: offset(%d), gpio_num(%d)\n", __func__, offset, gpio_num);
	return soc_gpio_get_in_value(gpio_num);
}

static int nexell_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned int gpio_num;
	struct gpio_bank *bank;

	bank = container_of(chip, struct gpio_bank, chip);
	gpio_num = bank->index * GPIO_NUM_PER_BANK + offset;
	soc_gpio_set_io_func(gpio_num, 0);	// GPIO PAD FUNC
	soc_gpio_set_io_dir(gpio_num, 1);
	soc_gpio_set_out_value(gpio_num, value);
	DBG("%s: offset(%d), gpio_num(%d), val(%d)\n", __func__, offset, gpio_num, value);
	return 0;
}

static void nexell_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned int gpio_num;
	struct gpio_bank *bank;

	bank = container_of(chip, struct gpio_bank, chip);
	gpio_num = bank->index * GPIO_NUM_PER_BANK + offset;
	soc_gpio_set_out_value(gpio_num, value);
	DBG("%s: offset(%d), gpio_num(%d), val(%d)\n", __func__, offset, gpio_num, value);
}

static struct gpio_bank nexell_gpio_bank[NUMBER_OF_GPIO_MODULE];
static int gpio_initialized = 0;
static int __init nexell_gpio_init(void)
{
	struct gpio_bank *bank;
	struct gpio_chip *chip;
	int i;

	if (gpio_initialized) {
		return 0;
	}

	for(i = 0; i < NUMBER_OF_GPIO_MODULE; i++) {
		bank = &nexell_gpio_bank[i];
		bank->index = i;
		chip = &bank->chip;
		chip->request = nexell_gpio_request;
		chip->free = nexell_gpio_free;
		chip->direction_input = nexell_gpio_direction_input;
		chip->get = nexell_gpio_get;
		chip->direction_output = nexell_gpio_direction_output;
		chip->set = nexell_gpio_set;
		chip->base = i * GPIO_NUM_PER_BANK;
		chip->label = "gpio";
		if(i == 5) {
			// alive
			chip->ngpio = 8;
		} else {
			chip->ngpio = GPIO_NUM_PER_BANK;
		}
		gpiochip_add(chip);
	}

	gpio_initialized = 1;
	printk("gpio: nexell generic gpio.\n");

	return 0;
}

arch_initcall(nexell_gpio_init);
