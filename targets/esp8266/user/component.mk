INC_DIRS += $(user_ROOT)..
INC_DIRS += $(user_ROOT)../../../jerry-core/include

#args for passing into compile rule generation
user_INC_DIR =
user_SRC_DIR = $(user_ROOT)

$(eval $(call component_compile_rules,user))
