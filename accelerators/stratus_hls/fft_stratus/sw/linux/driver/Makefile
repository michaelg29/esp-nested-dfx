include $(DRIVERS)/driver.mk


include ./Kbuild
ESPINCLUDE = $(EXTRA_CFLAGS)
export ESPINCLUDE

Module.symvers:
	$(MAKE) -C $(ESP_CORE_PATH) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH)
	cp $(ESP_CORE_PATH)/$@ .
