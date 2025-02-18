/*
 * LEGO MINSTORMS NXT analog sensor device driver
 *
 * Copyright (C) 2013-2015 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Note: The comment block below is used to generate docs on the ev3dev website.
 * Use kramdown (markdown) syntax. Use a '.' as a placeholder when blank lines
 * or leading whitespace is important for the markdown syntax.
 */

/**
 * DOC: website
 *
 * NXT Analog Sensor Driver
 *
 * The `nxt-analog-sensor` module provides all of the drivers for NXT/Analog
 * sensors. You can find the complete list [here][supported sensors]. These
 * drivers scale the analog voltage read from the sensor to a useful value.
 * .
 * ### sysfs
 * .
 * You can find the devices bound to this driver in the directory
 * `/sys/bus/lego/drivers/nxt-analog-sensor`. However, these drivers provide a
 * [lego-sensor device], which is where all the really useful attributes are.
 * .
 * `driver_names` (read-only)
 * : Returns a space-separated list of all NXT/Analog sensor driver names.
 * .
 * [lego-sensor device]: ../lego-sensor-class
 * [supported sensors]: /docs/sensors#supported-sensors
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <lego.h>
#include <lego_port_class.h>
#include <lego_sensor_class.h>

#include "nxt_analog_sensor.h"

static int nxt_analog_sensor_set_mode(void *context, u8 mode)
{
	struct nxt_analog_sensor_data *data = context;
	struct lego_sensor_mode_info *mode_info = &data->info.mode_info[mode];

	if (mode >= data->info.num_modes)
		return -EINVAL;

	data->ldev->port->nxt_analog_ops->set_pin5_gpio(data->ldev->port->context,
			data->info.analog_mode_info[mode].pin5_state);
	lego_port_set_raw_data_ptr_and_func(data->ldev->port, mode_info->raw_data,
		lego_sensor_get_raw_data_size(mode_info), NULL, NULL);

	return 0;
}

static int nxt_analog_sensor_probe(struct lego_device *ldev)
{
	struct nxt_analog_sensor_data *data;
	int err;

	if (WARN_ON(!ldev->entry_id))
		return -EINVAL;
	if (WARN_ON(!ldev->port->nxt_analog_ops))
		return -EINVAL;

	data = kzalloc(sizeof(struct nxt_analog_sensor_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->ldev = ldev;

	memcpy(&data->info, &nxt_analog_sensor_defs[ldev->entry_id->driver_data],
	       sizeof(struct nxt_analog_sensor_info));
	data->sensor.name = ldev->entry_id->name;
	data->sensor.port_name = ldev->port->port_name;
	data->sensor.num_modes	= data->info.num_modes;
	data->sensor.mode_info	= data->info.mode_info;
	data->sensor.set_mode	= nxt_analog_sensor_set_mode;
	data->sensor.context	= data;

	err = register_lego_sensor(&data->sensor, &ldev->dev);
	if (err)
		goto err_register_lego_sensor;

	dev_set_drvdata(&ldev->dev, data);
	nxt_analog_sensor_set_mode(data, 0);

	return 0;

err_register_lego_sensor:
	kfree(data);

	return err;
}

static int nxt_analog_sensor_remove(struct lego_device *ldev)
{
	struct nxt_analog_sensor_data *data = dev_get_drvdata(&ldev->dev);

	ldev->port->nxt_analog_ops->set_pin5_gpio(ldev->port->context,
		LEGO_PORT_GPIO_FLOAT);
	lego_port_set_raw_data_ptr_and_func(ldev->port, NULL, 0, NULL, NULL);
	unregister_lego_sensor(&data->sensor);
	dev_set_drvdata(&ldev->dev, NULL);
	kfree(data);
	return 0;
}

static struct lego_device_id nxt_analog_sensor_device_ids [] = {
	LEGO_DEVICE_ID(GENERIC_NXT_ANALOG_SENSOR),
	LEGO_DEVICE_ID(LEGO_NXT_TOUCH_SENSOR),
	LEGO_DEVICE_ID(LEGO_NXT_LIGHT_SENSOR),
	LEGO_DEVICE_ID(LEGO_NXT_SOUND_SENSOR),
	LEGO_DEVICE_ID(HT_EOPD_SENSOR),
	LEGO_DEVICE_ID(HT_FORCE_SENSOR),
	LEGO_DEVICE_ID(HT_GYRO_SENSOR),
	LEGO_DEVICE_ID(HT_MAGNETIC_SENSOR),
	LEGO_DEVICE_ID(MS_TOUCH_SENSOR_MUX),
	{ }
};

static ssize_t driver_names_show(struct device_driver *driver, char *buf)
{
	struct lego_device_id *id = nxt_analog_sensor_device_ids;
	int count = 0;

	while (id->name[0]) {
		count += sprintf(buf + count, "%s ", id->name);
		id++;
	}
	buf[count - 1] = '\n';

	return count;
}

DRIVER_ATTR_RO(driver_names);

static struct attribute *nxt_analog_driver_names_attrs[] = {
	&driver_attr_driver_names.attr,
	NULL,
};

ATTRIBUTE_GROUPS(nxt_analog_driver_names);

struct lego_device_driver nxt_analog_sensor_driver = {
	.probe	= nxt_analog_sensor_probe,
	.remove	= nxt_analog_sensor_remove,
	.driver = {
		.name	= "nxt-analog-sensor",
		.owner	= THIS_MODULE,
		.groups	= nxt_analog_driver_names_groups,
	},
	.id_table = nxt_analog_sensor_device_ids,
};
lego_device_driver(nxt_analog_sensor_driver);

MODULE_DESCRIPTION("LEGO MINSTORMS NXT analog sensor device driver");
MODULE_AUTHOR("David Lechner <david@lechnology.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("lego:nxt-analog-sensor");
