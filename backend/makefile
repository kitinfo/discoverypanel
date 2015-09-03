.PHONY all

all:
	@$(MAKE) -C spider
	@$(MAKE) -C updater

	$(MKDIR) bin
	@mv spider/spider bin/dp-spider
	@mv updater/updater bin/dp-updater

