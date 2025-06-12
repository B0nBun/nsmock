PWD = $(shell pwd)

OUTPUT_DIR=output
SOURCES=$(filter-out $(OUTPUT_DIR) Makefile Module.mk, $(wildcard *))
SYMLINKS=$(addprefix $(OUTPUT_DIR)/, $(SOURCES))

all: $(SYMLINKS) $(OUTPUT_DIR)/Makefile | $(OUTPUT_DIR)
	make -C /lib/modules/$(shell uname -r)/build M="$(PWD)/$(OUTPUT_DIR)" modules

$(OUTPUT_DIR):
	mkdir $(OUTPUT_DIR)

$(OUTPUT_DIR)/Makefile: Module.mk | $(OUTPUT_DIR)
	ln -s ../$< $@

$(OUTPUT_DIR)/%: % | $(OUTPUT_DIR)
	ln -s ../$< $@

clean:
	rm -rf $(OUTPUT_DIR)
	mkdir $(OUTPUT_DIR)
	
