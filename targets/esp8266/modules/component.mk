INC_DIRS += $(modules_ROOT).. $(modules_ROOT)include
INC_DIRS += $(modules_ROOT)../../../jerry-core/include

#args for passing into compile rule generation
modules_INC_DIR =
modules_SRC_DIR = $(modules_ROOT)

$(eval $(call component_compile_rules,modules))
