// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2020 Caleb Connolly <caleb@connolly.tech>
 * Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/backlight.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct sofef00_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	bool first_prepare;
	struct regulator_bulk_data supplies[3];
	struct gpio_desc *reset_gpio;
};

static inline
struct sofef00_panel *to_sofef00_panel(struct drm_panel *panel)
{
	return container_of(panel, struct sofef00_panel, panel);
}

static void sofef00_panel_reset(struct sofef00_panel *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(5000, 6000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(2000, 3000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(12000, 13000);
}

static int sofef00_panel_on(struct sofef00_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	usleep_range(10000, 11000);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0) {
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x07);
	mipi_dsi_dcs_write_seq(dsi, 0xb6, 0x12);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_POWER_SAVE, 0x00);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int sofef00_panel_off(struct sofef00_panel *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(40);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(160);

	return 0;
}

static int sofef00_panel_prepare(struct drm_panel *panel)
{
	struct sofef00_panel *ctx = to_sofef00_panel(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	/*
	 * On boot the panel has already been initialised, if the regulators are
	 * already enabled then we can safely assume that the panel is on and we
	 * can skip the prepare.
	 */
	if (regulator_is_enabled(ctx->supplies[0].consumer) && ctx->first_prepare) {
		ctx->first_prepare = false;
		dev_dbg(dev, "First prepare!\n");
		return 0;
	}

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulator: %d\n", ret);
		return ret;
	}

	sofef00_panel_reset(ctx);

	ret = sofef00_panel_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int sofef00_panel_disable(struct drm_panel *panel)
{
	struct sofef00_panel *ctx = to_sofef00_panel(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	ret = sofef00_panel_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode enchilada_panel_mode = {
	.clock = (1080 + 112 + 16 + 36) * (2280 + 36 + 8 + 12) * 60 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 112,
	.hsync_end = 1080 + 112 + 16,
	.htotal = 1080 + 112 + 16 + 36,
	.vdisplay = 2280,
	.vsync_start = 2280 + 36,
	.vsync_end = 2280 + 36 + 8,
	.vtotal = 2280 + 36 + 8 + 12,
	.width_mm = 68,
	.height_mm = 145,
};

static int sofef00_panel_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &enchilada_panel_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs sofef00_panel_panel_funcs = {
	.prepare = sofef00_panel_prepare,
	.disable = sofef00_panel_disable,
	.get_modes = sofef00_panel_get_modes,
};

static int sofef00_panel_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int err;
	u16 brightness = (u16)backlight_get_brightness(bl);

	err = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (err < 0)
		return err;

	return 0;
}

static const struct backlight_ops sofef00_panel_bl_ops = {
	.update_status = sofef00_panel_bl_update_status,
};

static struct backlight_device *
sofef00_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_PLATFORM,
		.brightness = 512,
		.max_brightness = 1023,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &sofef00_panel_bl_ops, &props);
}

static int sofef00_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct sofef00_panel *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "vddio";
	ctx->supplies[1].supply = "vci";
	ctx->supplies[2].supply = "poc";

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to get vddio regulator\n");

	/* Regulators are all boot-on, enable them to balance the refcounts so we can disable
	 * them later in the first prepare() call */
	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret,
				     "Failed to enable regulators\n");

	ctx->first_prepare = true;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;

	drm_panel_init(&ctx->panel, dev, &sofef00_panel_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = sofef00_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static void sofef00_panel_remove(struct mipi_dsi_device *dsi)
{
	struct sofef00_panel *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id sofef00_panel_of_match[] = {
	{ .compatible = "samsung,sofef00" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sofef00_panel_of_match);

static struct mipi_dsi_driver sofef00_panel_driver = {
	.probe = sofef00_panel_probe,
	.remove = sofef00_panel_remove,
	.driver = {
		.name = "panel-samsung-sofef00",
		.of_match_table = sofef00_panel_of_match,
	},
};

module_mipi_dsi_driver(sofef00_panel_driver);

MODULE_AUTHOR("Caleb Connolly <caleb@connolly.tech>");
MODULE_DESCRIPTION("DRM driver for Samsung AMOLED DSI panels found in OnePlus 6/6T phones");
MODULE_LICENSE("GPL v2");