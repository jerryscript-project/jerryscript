export SHELL=/bin/bash

ifeq ($(TARGET),)
  $(error TARGET not set)
endif

ENGINE_NAME ?= jerry

# Parsing target
#   '.' -> ' '
TARGET_SPACED := $(subst ., ,$(TARGET))
#   extract target mode part
TARGET_MODE   := $(word 1,$(TARGET_SPACED))
#   extract target system part with modifiers
TARGET_SYSTEM_AND_MODS := $(word 2,$(TARGET_SPACED))
TARGET_SYSTEM_AND_MODS_SPACED := $(subst -, ,$(TARGET_SYSTEM_AND_MODS))

#   extract target system part
TARGET_SYSTEM := $(word 1,$(TARGET_SYSTEM_AND_MODS_SPACED))

#   extract modifiers
TARGET_MODS := $(wordlist 2, $(words $(TARGET_SYSTEM_AND_MODS_SPACED)), $(TARGET_SYSTEM_AND_MODS_SPACED))

#   extract optional action part
TARGET_ACTION := $(word 3,$(TARGET_SPACED))

# Target used as dependency of an action (check, flash, etc.)
TARGET_OF_ACTION := $(TARGET_MODE).$(TARGET_SYSTEM_AND_MODS)

# target folder name in $(OUT_DIR)
TARGET_DIR=$(OUT_DIR)/$(TARGET_MODE).$(TARGET_SYSTEM_AND_MODS)

# unittests mode -> linux system
ifeq ($(TARGET_MODE),unittests_run)
 TARGET_SYSTEM := linux
 TARGET_SYSTEM_AND_MODS := $(TARGET_SYSTEM)
 TARGET_DIR := $(OUT_DIR)/unittests
endif

ifeq ($(filter valgrind,$(TARGET_MODS)), valgrind)
     OPTION_VALGRIND := enable

     ifeq ($(OPTION_SANITIZE),enable)
      $(error ASAN and Valgrind are mutually exclusive)
     endif
else
     OPTION_VALGRIND := disable
endif

ifeq ($(OPTION_VALGRIND),enable)
 VALGRIND_CMD := "valgrind --error-exitcode=254 --track-origins=yes"
 VALGRIND_TIMEOUT := 60
else
 VALGRIND_CMD :=
 VALGRIND_TIMEOUT :=
endif

.PHONY: unittests_run $(CHECK_TARGETS)

unittests_run:
	@ VALGRIND=$(VALGRIND_CMD) ./tools/runners/run-unittests.sh $(TARGET_DIR) $(TESTS_OPTS)

$(CHECK_TARGETS):
	@ if [ ! -f $(TARGET_DIR)/$(ENGINE_NAME) ]; then echo $(TARGET_OF_ACTION) is not built yet; exit 1; fi;
	@ if [[ ! -d "$(TESTS)" && ! -f "$(TESTS)" ]]; then echo \"$(TESTS)\" is not a directory and not a file; exit 1; fi;
	@ rm -rf $(TARGET_DIR)/check
	@ mkdir -p $(TARGET_DIR)/check
	@ if [ "$(OUTPUT_TO_LOG)" = "enable" ]; \
          then \
            ADD_OPTS="--output-to-log"; \
          fi; \
          VALGRIND=$(VALGRIND_CMD) TIMEOUT=$(VALGRIND_TIMEOUT) ./tools/runners/run-test-pass.sh $(TARGET_DIR)/$(ENGINE_NAME) $(TARGET_DIR)/check $(TESTS) $(TESTS_OPTS) $$ADD_OPTS; \
          status_code=$$?; \
          if [ $$status_code -ne 0 ]; \
          then \
            echo $(TARGET) failed; \
            if [ "$(OUTPUT_TO_LOG)" = "enable" ]; \
            then \
              echo See log in $(TARGET_DIR)/check directory for details.; \
            fi; \
            \
            exit $$status_code; \
          fi; \
          if [ -d $(TESTS_DIR)/fail/ ]; \
          then \
            VALGRIND=$(VALGRIND_CMD) TIMEOUT=$(VALGRIND_TIMEOUT) ./tools/runners/run-test-xfail.sh $(TARGET_DIR)/$(ENGINE_NAME) $(TARGET_DIR)/check $(PARSER_ERROR_CODE) $(TESTS_DIR) $(TESTS_OPTS) $$ADD_OPTS; \
            status_code=$$?; \
            if [ $$status_code -ne 0 ]; \
            then \
              echo $(TARGET) failed; \
              if [ "$(OUTPUT_TO_LOG)" = "enable" ]; \
              then \
                echo See log in $(TARGET_DIR)/check directory for details.; \
              fi; \
              \
              exit $$status_code; \
            fi; \
          fi;
