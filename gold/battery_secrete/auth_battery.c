#include <linux/module.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/delay.h>

#include "battery_auth_class.h"
#include "mtk_battery.h"

enum {
	MAIN_SUPPLY = 0,
	SECEON_SUPPLY,
/* N17 code for HQ-295595 by tongjiacheng at 20230509 start*/
	THIRD_SUPPLY,
/* N17 code for HQ-295595 by tongjiacheng at 20230509 end*/
	MAX_SUPPLY,
};

/* N17 code for HQ-295595 by tongjiacheng at 20230509 start*/
static const char *auth_device_name[] = {
	"main_suppiler",
	"second_supplier",
	"third_supplier",
	"unknown",
};
/* N17 code for HQ-295595 by tongjiacheng at 20230509 end*/

struct auth_data {
	struct auth_device *auth_dev[MAX_SUPPLY];

	struct power_supply *verify_psy;
	struct power_supply_desc desc;

	struct delayed_work dwork;
	struct delayed_work fg_cycle_dwork;

	bool auth_result;
/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
	u8 batt_id;
/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */

	/* the battery's cycle count, kept in the authentication chip */
	int old_fg_cycle;
	int new_fg_cycle;
	u32 batt_cycle;
	struct mutex cycle_lock;
	struct power_supply *bat_psy;
};

static struct auth_data *g_info;
/* N17 code for HQ-306004 by liunianliang at 2023/07/05 start */
static int auth_index = 0;
/* N17 code for HQ-306004 by liunianliang at 2023/07/05 end */

static enum power_supply_property verify_props[] = {
	POWER_SUPPLY_PROP_AUTHENTIC,
/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
	POWER_SUPPLY_PROP_TYPE,
/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
/* N17 code for HQ-306004 by liunianliang at 2023/07/05 start */
	POWER_SUPPLY_PROP_MANUFACTURER,
/* N17 code for HQ-306004 by liunianliang at 2023/07/05 end */
	POWER_SUPPLY_PROP_CYCLE_COUNT,
};

static int verify_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct auth_data *info = power_supply_get_drvdata(psy);
	pr_info("%s:%d\n", __func__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_AUTHENTIC:
		val->intval = info->auth_result;
		break;
/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = info->batt_id;
/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
		break;
/* N17 code for HQ-306004 by liunianliang at 2023/07/05 start */
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = auth_device_name[auth_index];
		break;
/* N17 code for HQ-306004 by liunianliang at 2023/07/05 end */
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		if (g_info) {
			val->intval = g_info->batt_cycle;
		} else {
			pr_err("%s: g_info is NULL \n", __func__);
			val->intval = 0;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * User space writes the gauge's cycle count to cycle_count; the work
 * below then adds what the gauge counted since its last update to the
 * count in the authentication chip, which reports it back. The work
 * takes the gauge's count from the battery supply, whatever was written.
 */
static int verify_set_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       const union power_supply_propval *val)
{
	struct auth_data *info = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		cancel_delayed_work_sync(&info->fg_cycle_dwork);
		info->new_fg_cycle = val->intval;
		pr_info("%s[%d] set new_fg_cycle = %d", __func__, __LINE__, info->new_fg_cycle);
		schedule_delayed_work(&info->fg_cycle_dwork, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int verify_is_writeable(struct power_supply *psy,
			       enum power_supply_property psp)
{
	return psp == POWER_SUPPLY_PROP_CYCLE_COUNT;
}

/* N17 code for HQ-292265 by tongjiacheng at 20230523 start */
#define AUTHENTIC_COUNT_MAX 3
/* N17 code for HQ-292265 by tongjiacheng at 20230523 end */
static void auth_battery_dwork(struct work_struct *work)
{
	int i = 0;
	struct auth_data *info = container_of(to_delayed_work(work),
					      struct auth_data, dwork);

	int authen_result;
	static int retry_authentic = 0;

	for (i = 0; i < MAX_SUPPLY; i++) {
		if (!info->auth_dev[i])
			continue;
		authen_result = auth_device_start_auth(info->auth_dev[i]);
		if (!authen_result) {
		/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */	
			auth_device_get_batt_id(info->auth_dev[i], &(info->batt_id));
		/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
		/* N17 code for HQ-306004 by liunianliang at 2023/07/05 start */
			auth_index = i;
		/* N17 code for HQ-306004 by liunianliang at 2023/07/05 end */
			break;
		}
	}

	if (i == MAX_SUPPLY) {
		retry_authentic++;
		if (retry_authentic < AUTHENTIC_COUNT_MAX) {
			pr_info
			    ("battery authentic work begin to restart %d\n",
			     retry_authentic);
			schedule_delayed_work(&(info->dwork),
					      msecs_to_jiffies(5000));
		}
		if (retry_authentic == AUTHENTIC_COUNT_MAX) {
			pr_info("authentic result is %s\n",
				(authen_result == 0) ? "success" : "fail");
		/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
			info->batt_id = 0xff;
		/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
		}
	} else
		pr_info("authentic result is %s\n",
			(authen_result == 0) ? "success" : "fail");

	info->auth_result = ((authen_result == 0) ? true : false);
}

#define EEA_CYCLE_COUNT_MIN 480
static void update_fg_cycle_work(struct work_struct *work)
{
	struct auth_data *info = container_of(to_delayed_work(work),
					      struct auth_data, fg_cycle_dwork);
	struct mtk_battery_manager *bm;
	u32 batt_cycle, read_cycle, new_batt_cycle;
	int fg_cycle, delta_cycle;
	int ret;

	if (!info->bat_psy)
		info->bat_psy = power_supply_get_by_name("battery");
	if (IS_ERR_OR_NULL(info->bat_psy)) {
		pr_info("%s:fail to get bat_psy\n", __func__);
		goto retry;
	}

	bm = power_supply_get_drvdata(info->bat_psy);
	if (bm->gm1->fake_bat_cycle > 0) {
		pr_err("fake_bat_cycle is %d, update_fg_cycle_work return\n",
		       bm->gm1->fake_bat_cycle);
		return;
	}

	ret = auth_device_get_cycle_count(info->auth_dev[auth_index], &batt_cycle);
	if (ret == -EOPNOTSUPP) {
		pr_err("get cycle count API is NULL\n");
		return;
	} else if (ret) {
		pr_err("update cc to 480: read auth cycle count error\n");
		goto retry;
	}

	if (batt_cycle < EEA_CYCLE_COUNT_MIN) {
		ret = auth_device_set_cycle_count(info->auth_dev[auth_index],
						  EEA_CYCLE_COUNT_MIN, batt_cycle);
		if (ret) {
			pr_err("update cc to 480: write auth cycle count error\n");
			goto retry;
		}
		ret = auth_device_get_cycle_count(info->auth_dev[auth_index], &batt_cycle);
		if (ret) {
			pr_err("update cc to 480: read auth cycle count error2\n");
			goto retry;
		}
		if (batt_cycle < EEA_CYCLE_COUNT_MIN)
			pr_err("update cc to 480: read auth cycle count less than 480\n");
		else
			pr_err("update cc to 480: success\n");
		goto retry;
	}

	info->new_fg_cycle = bm->gm1->bat_cycle;
	mutex_lock(&info->cycle_lock);
	fg_cycle = info->new_fg_cycle;
	if (info->old_fg_cycle < 0) {
		info->old_fg_cycle = fg_cycle;
		ret = auth_device_get_cycle_count(info->auth_dev[auth_index], &batt_cycle);
		if (ret) {
			pr_err("read auth cycle count error\n");
			info->batt_cycle = fg_cycle;
			goto unlock;
		}
		info->batt_cycle = batt_cycle;
		pr_info("read auth cycle count success\n");
		pr_info("success to init battery cycle, batt_cycle = %d, fg_cycle = %d\n",
			info->batt_cycle, info->old_fg_cycle);
	}

	mdelay(20);
	delta_cycle = fg_cycle - info->old_fg_cycle;
	pr_info("new_fg_cycle = %d, old_fg_cycle = %d, batt_cycle = %d, delta_cycle = %d\n",
		fg_cycle, info->old_fg_cycle, info->batt_cycle, delta_cycle);

	ret = auth_device_get_cycle_count(info->auth_dev[auth_index], &batt_cycle);
	if (ret) {
		pr_err("first read auth cycle count error\n");
		goto unlock;
	}
	mdelay(20);
	new_batt_cycle = batt_cycle + delta_cycle;
	ret = auth_device_set_cycle_count(info->auth_dev[auth_index],
					  new_batt_cycle, batt_cycle);
	if (ret) {
		pr_err("write auth cycle count error\n");
		goto unlock;
	}
	mdelay(20);
	ret = auth_device_get_cycle_count(info->auth_dev[auth_index], &read_cycle);
	if (ret) {
		pr_err("second read auth cycle count error\n");
		goto unlock;
	}
	if (new_batt_cycle != read_cycle) {
		pr_err("update cycle fail, retry update cycle\n");
		goto unlock;
	}

	info->old_fg_cycle = fg_cycle;
	info->batt_cycle = new_batt_cycle;
	pr_info("[EEA] success to update cycle count, batt_cycle = %d, fg_cycle = %d\n",
		info->batt_cycle, info->old_fg_cycle);
	mutex_unlock(&info->cycle_lock);
	return;

unlock:
	mutex_unlock(&info->cycle_lock);
retry:
	schedule_delayed_work(&info->fg_cycle_dwork, msecs_to_jiffies(5000));
}

static int __init auth_battery_init(void)
{
	int ret = 0;
	int i = 0;
	struct auth_data *info;
	struct power_supply_config cfg = { };

	pr_info("%s enter\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
/* N17 code for HQ-295595 by tongjiacheng at 20230509 start*/
	for (i = 0; i < MAX_SUPPLY; i++) {
		info->auth_dev[i] = get_batt_auth_by_name(auth_device_name[i]);
		if (!info->auth_dev[i])
			break;
	}
/* N17 code for HQ-295595 by tongjiacheng at 20230509 end*/
/* N17 code for HQ-293392 by tongjiacheng at 20230428 end */
	cfg.drv_data = info;
	info->desc.name = "batt_verify";
	info->desc.type = POWER_SUPPLY_TYPE_BATTERY;
	info->desc.properties = verify_props;
	info->desc.num_properties = ARRAY_SIZE(verify_props);
	info->desc.get_property = verify_get_property;
	info->desc.set_property = verify_set_property;
	info->desc.property_is_writeable = verify_is_writeable;
	info->verify_psy =
	    power_supply_register(NULL, &(info->desc), &cfg);
	if (!info->verify_psy)
		pr_err("%s register verify psy fail\n", __func__);

	info->old_fg_cycle = -1;
	INIT_DELAYED_WORK(&info->dwork, auth_battery_dwork);
	INIT_DELAYED_WORK(&info->fg_cycle_dwork, update_fg_cycle_work);
/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */
	info->batt_id = 0xff;
/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
	g_info = info;

	for (i = 0; i < MAX_SUPPLY; i++) {
		if (!info->auth_dev[i])
			continue;
		ret = auth_device_start_auth(info->auth_dev[i]);
		if (!ret) {
		/* N17 code for HQ-292265 by tongjiacheng at 20230515 start */	
			auth_device_get_batt_id(info->auth_dev[i], &(info->batt_id));
		/* N17 code for HQ-292265 by tongjiacheng at 20230515 end */
		/* N17 code for HQ-306004 by liunianliang at 2023/07/05 start */
			auth_index = i;
		/* N17 code for HQ-306004 by liunianliang at 2023/07/05 end */
			break;
		}
	}
	if (i >= MAX_SUPPLY)
		schedule_delayed_work(&info->dwork, msecs_to_jiffies(500));
	else
		info->auth_result = true;

	return 0;
}

static void __exit auth_battery_exit(void)
{
	int i = 0;

	power_supply_unregister(g_info->verify_psy);

	for (i = 0; i < MAX_SUPPLY; i++)
		auth_device_unregister(g_info->auth_dev[i]);

	kfree(g_info);
}

module_init(auth_battery_init);
module_exit(auth_battery_exit);
MODULE_LICENSE("GPL");
