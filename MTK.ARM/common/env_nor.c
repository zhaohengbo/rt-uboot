#include <common.h>
#include <environment.h>
#include <malloc.h>
#include <search.h>
#include <errno.h>
#include "../drivers/flash/mtk_nor.h"

DECLARE_GLOBAL_DATA_PTR;

char *env_name_spec = "SPI NOR Flash";

int saveenv(void)
{
	u32	saved_size, saved_offset, sector = 1;
	char	*res, *saved_buffer = NULL;
	int	ret = 1;
	env_t	env_new;
	ssize_t	len;
	size_t	retlen;

	/* Is the sector larger than the env (i.e. embedded) */
	if (CONFIG_ENV_SECT_SIZE > CONFIG_ENV_SIZE) {
		saved_size = CONFIG_ENV_SECT_SIZE - CONFIG_ENV_SIZE;
		saved_offset = CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE;
		saved_buffer = malloc(saved_size);
		if (!saved_buffer)
			goto done;

		ret = mtk_nor_read(saved_offset,
			saved_size, &retlen, saved_buffer);
		if (ret)
			goto done;
	}

	if (CONFIG_ENV_SIZE > CONFIG_ENV_SECT_SIZE) {
		sector = CONFIG_ENV_SIZE / CONFIG_ENV_SECT_SIZE;
		if (CONFIG_ENV_SIZE % CONFIG_ENV_SECT_SIZE)
			sector++;
	}

	res = (char *)&env_new.data;
	len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
	if (len < 0) {
		error("Cannot export environment: errno = %d\n", errno);
		goto done;
	}
	env_new.crc = crc32(0, env_new.data, ENV_SIZE);

	puts("Erasing SPI NOR flash...");
	ret = mtk_nor_erase(CONFIG_ENV_OFFSET,
		sector * CONFIG_ENV_SECT_SIZE);
	if (ret)
		goto done;

	puts("Writing to SPI NOR flash...");
	mtk_nor_write(CONFIG_ENV_OFFSET, 
		CONFIG_ENV_SIZE, &retlen, &env_new);
	if (ret)
		goto done;

	if (CONFIG_ENV_SECT_SIZE > CONFIG_ENV_SIZE) {
		ret = mtk_nor_write(saved_offset,
			saved_size, &retlen, saved_buffer);
		if (ret)
			goto done;
	}

	ret = 0;
	puts("done\n");

 done:
	if (saved_buffer)
		free(saved_buffer);

	return ret;
}

void env_relocate_spec(void)
{
	int ret;
	char *buf = NULL;
	size_t	retlen;

	buf = (char *)malloc(CONFIG_ENV_SIZE);

	ret = mtk_nor_read(CONFIG_ENV_OFFSET,
				CONFIG_ENV_SIZE, &retlen, buf);
	if (ret) {
		set_default_env("!spi_flash_read() failed");
		goto out;
	}

	ret = env_import(buf, 1);
	if (ret)
		gd->env_valid = 1;
out:
	if (buf)
		free(buf);
}

int env_init(void)
{
	/* SPI flash isn't usable before relocation */
	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return 0;
}
