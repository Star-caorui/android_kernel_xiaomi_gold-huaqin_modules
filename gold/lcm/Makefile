# SPDX-License-Identifier: GPL-2.0
# the panels include "../mediatek/..." from the device modules' drm/panel directory
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/gpu/drm/panel
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/include/mt-plat

ifneq ($(filter y m, $(CONFIG_MTK_MME_SUPPORT)),)
ccflags-y += -I$(DEVICE_MODULES_PATH)/drivers/misc/mediatek/mme
endif

# panel_ddic_send_cmd() keeps its 25 commands (6 KiB) on the stack
CFLAGS_panel-n17-36-02-0a-dsc-vdo.o += -Wframe-larger-than=8192
CFLAGS_panel-n17-36-0f-0c-dsc-vdo.o += -Wframe-larger-than=8192
CFLAGS_panel-n17-42-0d-0b-dsc-vdo.o += -Wframe-larger-than=8192

obj-$(CONFIG_DRM_PANEL_N17_36_02_0A_DSC_VDO) += panel-n17-36-02-0a-dsc-vdo.o
obj-$(CONFIG_DRM_PANEL_N17_36_0F_0C_DSC_VDO) += panel-n17-36-0f-0c-dsc-vdo.o
obj-$(CONFIG_DRM_PANEL_N17_42_0D_0B_DSC_VDO) += panel-n17-42-0d-0b-dsc-vdo.o
