APPLICATIONS := daybreak haze reboot_to_hekate reboot_to_payload

SUBFOLDERS := $(APPLICATIONS)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
