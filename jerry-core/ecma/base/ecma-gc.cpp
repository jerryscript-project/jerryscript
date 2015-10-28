/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmagc Garbage collector
 * @{
 */

/**
 * Garbage collector implementation
 */

#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "jrt-bit-fields.h"
#include "vm-stack.h"

#define JERRY_INTERNAL
#include "jerry-internal.h"

/**
 * TODO:
 *      Extract GC to a separate component
 */

/**
 * An object's GC color
 *
 * Tri-color marking:
 *   WHITE_GRAY, unvisited -> WHITE // not referenced by a live object or the reference not found yet
 *   WHITE_GRAY, visited   -> GRAY  // referenced by some live object
 *   BLACK                 -> BLACK // all referenced objects are gray or black
 */
typedef enum
{
  ECMA_GC_COLOR_WHITE_GRAY, /**< white or gray */
  ECMA_GC_COLOR_BLACK, /**< black */
  ECMA_GC_COLOR__COUNT /**< number of colors */
} ecma_gc_color_t;

/**
 * List of marked (visited during current GC session) and umarked objects
 */
static ecma_object_t *ecma_gc_objects_lists[ECMA_GC_COLOR__COUNT];

/**
 * Current state of an object's visited flag that indicates whether the object is in visited state:
 *  visited_field | visited_flip_flag | real_value
 *         false  |            false  |     false
 *         false  |             true  |      true
 *          true  |            false  |      true
 *          true  |             true  |     false
 */
static bool ecma_gc_visited_flip_flag = false;

/**
 * Number of currently allocated objects
 */
static size_t ecma_gc_objects_number = 0;

/**
 * Number of newly allocated objects since last GC session
 */
static size_t ecma_gc_new_objects_since_last_gc = 0;

static void ecma_gc_mark (ecma_object_t *object_p);
static void ecma_gc_sweep (ecma_object_t *object_p);

/**
 * Get GC reference counter of the object.
 */
static uint32_t
ecma_gc_get_object_refs (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);

  return (uint32_t) jrt_extract_bit_field (object_p->container,
                                           ECMA_OBJECT_GC_REFS_POS,
                                           ECMA_OBJECT_GC_REFS_WIDTH);
} /* ecma_gc_get_object_refs */

/**
 * Set GC reference counter of the object.
 */
static void
ecma_gc_set_object_refs (ecma_object_t *object_p, /**< object */
                         uint32_t refs) /**< new reference counter */
{
  JERRY_ASSERT (object_p != NULL);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 refs,
                                                 ECMA_OBJECT_GC_REFS_POS,
                                                 ECMA_OBJECT_GC_REFS_WIDTH);
} /* ecma_gc_set_object_refs */

/**
 * Get next object in list of objects with same generation.
 */
static ecma_object_t*
ecma_gc_get_object_next (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_GC_NEXT_CP_WIDTH);
  uintptr_t next_cp = (uintptr_t) jrt_extract_bit_field (object_p->container,
                                                         ECMA_OBJECT_GC_NEXT_CP_POS,
                                                         ECMA_OBJECT_GC_NEXT_CP_WIDTH);

  return ECMA_GET_POINTER (ecma_object_t,
                           next_cp);
} /* ecma_gc_get_object_next */

/**
 * Set next object in list of objects with same generation.
 */
static void
ecma_gc_set_object_next (ecma_object_t *object_p, /**< object */
                         ecma_object_t *next_object_p) /**< next object */
{
  JERRY_ASSERT (object_p != NULL);

  uintptr_t next_cp;
  ECMA_SET_POINTER (next_cp, next_object_p);

  JERRY_ASSERT (sizeof (uintptr_t) * JERRY_BITSINBYTE >= ECMA_OBJECT_GC_NEXT_CP_WIDTH);
  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 next_cp,
                                                 ECMA_OBJECT_GC_NEXT_CP_POS,
                                                 ECMA_OBJECT_GC_NEXT_CP_WIDTH);
} /* ecma_gc_set_object_next */

/**
 * Get visited flag of the object.
 */
static bool
ecma_gc_is_object_visited (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);

  bool flag_value = (bool) jrt_extract_bit_field (object_p->container,
                                                  ECMA_OBJECT_GC_VISITED_POS,
                                                  ECMA_OBJECT_GC_VISITED_WIDTH);

  return (flag_value != ecma_gc_visited_flip_flag);
} /* ecma_gc_is_object_visited */

/**
 * Set visited flag of the object.
 */
static void
ecma_gc_set_object_visited (ecma_object_t *object_p, /**< object */
                            bool is_visited) /**< flag value */
{
  JERRY_ASSERT (object_p != NULL);

  if (ecma_gc_visited_flip_flag)
  {
    is_visited = !is_visited;
  }

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 is_visited,
                                                 ECMA_OBJECT_GC_VISITED_POS,
                                                 ECMA_OBJECT_GC_VISITED_WIDTH);
} /* ecma_gc_set_object_visited */

/**
 * Initialize GC information for the object
 */
void
ecma_init_gc_info (ecma_object_t *object_p) /**< object */
{
  ecma_gc_objects_number++;
  ecma_gc_new_objects_since_last_gc++;

  JERRY_ASSERT (ecma_gc_new_objects_since_last_gc <= ecma_gc_objects_number);

  ecma_gc_set_object_refs (object_p, 1);

  ecma_gc_set_object_next (object_p, ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY]);
  ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY] = object_p;

  /* Should be set to false at the beginning of garbage collection */
  ecma_gc_set_object_visited (object_p, false);
} /* ecma_init_gc_info */

/**
 * Increase reference counter of an object
 */
void
ecma_ref_object (ecma_object_t *object_p) /**< object */
{
  ecma_gc_set_object_refs (object_p, ecma_gc_get_object_refs (object_p) + 1);
} /* ecma_ref_object */

/**
 * Decrease reference counter of an object
 */
void
ecma_deref_object (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (ecma_gc_get_object_refs (object_p) > 0);
  ecma_gc_set_object_refs (object_p, ecma_gc_get_object_refs (object_p) - 1);
} /* ecma_deref_object */

/**
 * Initialize garbage collector
 */
void
ecma_gc_init (void)
{
  ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY] = NULL;
  ecma_gc_objects_lists[ECMA_GC_COLOR_BLACK] = NULL;
} /* ecma_gc_init */

/**
 * Mark objects as visited starting from specified object as root
 */
void
ecma_gc_mark (ecma_object_t *object_p) /**< object to mark from */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (ecma_gc_is_object_visited (object_p));

  bool traverse_properties = true;

  if (ecma_is_lexical_environment (object_p))
  {
    ecma_object_t *lex_env_p = ecma_get_lex_env_outer_reference (object_p);
    if (lex_env_p != NULL)
    {
      ecma_gc_set_object_visited (lex_env_p, true);
    }

    if (ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND)
    {
      ecma_object_t *binding_object_p = ecma_get_lex_env_binding_object (object_p);
      ecma_gc_set_object_visited (binding_object_p, true);

      traverse_properties = false;
    }
  }
  else
  {
    ecma_object_t *proto_p = ecma_get_object_prototype (object_p);
    if (proto_p != NULL)
    {
      ecma_gc_set_object_visited (proto_p, true);
    }
  }

  if (traverse_properties)
  {
    for (ecma_property_t *property_p = ecma_get_property_list (object_p), *next_property_p;
         property_p != NULL;
         property_p = next_property_p)
    {
      next_property_p = ECMA_GET_POINTER (ecma_property_t,
                                          property_p->next_property_p);

      switch ((ecma_property_type_t) property_p->type)
      {
        case ECMA_PROPERTY_NAMEDDATA:
        {
          ecma_value_t value = ecma_get_named_data_property_value (property_p);

          if (ecma_is_value_object (value))
          {
            ecma_object_t *value_obj_p = ecma_get_object_from_value (value);

            ecma_gc_set_object_visited (value_obj_p, true);
          }

          break;
        }

        case ECMA_PROPERTY_NAMEDACCESSOR:
        {
          ecma_object_t *getter_obj_p = ecma_get_named_accessor_property_getter (property_p);
          ecma_object_t *setter_obj_p = ecma_get_named_accessor_property_setter (property_p);

          if (getter_obj_p != NULL)
          {
            ecma_gc_set_object_visited (getter_obj_p, true);
          }

          if (setter_obj_p != NULL)
          {
            ecma_gc_set_object_visited (setter_obj_p, true);
          }

          break;
        }

        case ECMA_PROPERTY_INTERNAL:
        {
          ecma_internal_property_id_t property_id = (ecma_internal_property_id_t) property_p->u.internal_property.type;
          uint32_t property_value = property_p->u.internal_property.value;

          switch (property_id)
          {
            case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* a collection of ecma-values */
            case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* a collection of ecma-values */
            {
              JERRY_UNIMPLEMENTED ("Indexed array storage is not implemented yet.");
            }

            case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t
                                                        (see above in the routine) */
            case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t
                                                         (see above in the routine) */
            case ECMA_INTERNAL_PROPERTY__COUNT: /* not a real internal property type,
                                                 * but number of the real internal property types */
            {
              JERRY_UNREACHABLE ();
            }

            case ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS: /* a collection of strings */
            case ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE: /* compressed pointer to a ecma_string_t */
            case ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE: /* compressed pointer to a ecma_number_t */
            case ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE: /* a simple boolean value */
            case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
            case ECMA_INTERNAL_PROPERTY_CODE_BYTECODE: /* compressed pointer to a bytecode array */
            case ECMA_INTERNAL_PROPERTY_CODE_FLAGS_AND_OFFSET: /* an integer */
            case ECMA_INTERNAL_PROPERTY_NATIVE_CODE: /* an external pointer */
            case ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE: /* an external pointer */
            case ECMA_INTERNAL_PROPERTY_FREE_CALLBACK: /* an object's native free callback */
            case ECMA_INTERNAL_PROPERTY_BUILT_IN_ID: /* an integer */
            case ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_DESC: /* an integer */
            case ECMA_INTERNAL_PROPERTY_EXTENSION_ID: /* an integer */
            case ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31: /* an integer (bit-mask) */
            case ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63: /* an integer (bit-mask) */
            case ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE:
            {
              break;
            }

            case ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_THIS: /* an ecma-value */
            {
              if (ecma_is_value_object (property_value))
              {
                ecma_object_t *obj_p = ecma_get_object_from_value (property_value);

                ecma_gc_set_object_visited (obj_p, true);
              }

              break;
            }

            case ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_ARGS: /* a collection of ecma-values */
            {
              ecma_collection_header_t *bound_arg_list_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                      property_value);

              ecma_collection_iterator_t bound_args_iterator;
              ecma_collection_iterator_init (&bound_args_iterator, bound_arg_list_p);

              for (ecma_length_t i = 0; i < bound_arg_list_p->unit_number; i++)
              {
                bool is_moved = ecma_collection_iterator_next (&bound_args_iterator);
                JERRY_ASSERT (is_moved);

                if (ecma_is_value_object (*bound_args_iterator.current_value_p))
                {
                  ecma_object_t *obj_p = ecma_get_object_from_value (*bound_args_iterator.current_value_p);

                  ecma_gc_set_object_visited (obj_p, true);
                }
              }

              break;
            }

            case ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION: /* an object */
            case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
            case ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP: /* an object */
            {
              ecma_object_t *obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, property_value);

              ecma_gc_set_object_visited (obj_p, true);

              break;
            }
          }

          break;
        }
      }
    }
  }
} /* ecma_gc_mark */

/**
 * Free specified object
 */
void
ecma_gc_sweep (ecma_object_t *object_p) /**< object to free */
{
  JERRY_ASSERT (object_p != NULL
                && !ecma_gc_is_object_visited (object_p)
                && ecma_gc_get_object_refs (object_p) == 0);

  if (!ecma_is_lexical_environment (object_p))
  {
    /* if the object provides free callback, invoke it with handle stored in the object */

    ecma_external_pointer_t freecb_p;
    ecma_external_pointer_t native_p;

    bool is_retrieved = ecma_get_external_pointer_value (object_p,
                                                         ECMA_INTERNAL_PROPERTY_FREE_CALLBACK,
                                                         &freecb_p);
    if (is_retrieved)
    {
      is_retrieved = ecma_get_external_pointer_value (object_p,
                                                      ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE,
                                                      &native_p);
      JERRY_ASSERT (is_retrieved);

      jerry_dispatch_object_free_callback (freecb_p, native_p);
    }
  }

  if (!ecma_is_lexical_environment (object_p) ||
      ecma_get_lex_env_type (object_p) != ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND)
  {
    for (ecma_property_t *property = ecma_get_property_list (object_p), *next_property_p;
         property != NULL;
         property = next_property_p)
    {
      next_property_p = ECMA_GET_POINTER (ecma_property_t,
                                          property->next_property_p);

      ecma_free_property (object_p, property);
    }
  }

  JERRY_ASSERT (ecma_gc_objects_number > 0);
  ecma_gc_objects_number--;

  ecma_dealloc_object (object_p);
} /* ecma_gc_sweep */

/**
 * Run garbage collecting
 */
void
ecma_gc_run (void)
{
  ecma_gc_new_objects_since_last_gc = 0;

  JERRY_ASSERT (ecma_gc_objects_lists[ECMA_GC_COLOR_BLACK] == NULL);

  /* if some object is referenced from stack or globals (i.e. it is root), mark it */
  for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY];
       obj_iter_p != NULL;
       obj_iter_p = ecma_gc_get_object_next (obj_iter_p))
  {
    JERRY_ASSERT (!ecma_gc_is_object_visited (obj_iter_p));

    if (ecma_gc_get_object_refs (obj_iter_p) > 0)
    {
      ecma_gc_set_object_visited (obj_iter_p, true);
    }
  }

  /* if some object is referenced from a register variable (i.e. it is root),
   * start recursive marking traverse from the object */
  for (vm_stack_frame_t *frame_iter_p = vm_stack_get_top_frame ();
       frame_iter_p != NULL;
       frame_iter_p = frame_iter_p->prev_frame_p)
  {
    for (uint32_t reg_index = 0; reg_index < frame_iter_p->regs_number; reg_index++)
    {
      ecma_value_t reg_value = vm_stack_frame_get_reg_value (frame_iter_p, VM_REG_FIRST + reg_index);

      if (ecma_is_value_object (reg_value))
      {
        ecma_object_t *obj_p = ecma_get_object_from_value (reg_value);

        ecma_gc_set_object_visited (obj_p, true);
      }
    }
  }

  bool marked_anything_during_current_iteration = false;

  do
  {
    marked_anything_during_current_iteration = false;

    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY], *obj_prev_p = NULL, *obj_next_p;
         obj_iter_p != NULL;
         obj_iter_p = obj_next_p)
    {
      obj_next_p = ecma_gc_get_object_next (obj_iter_p);

      if (ecma_gc_is_object_visited (obj_iter_p))
      {
        /* Moving the object to list of marked objects */
        ecma_gc_set_object_next (obj_iter_p, ecma_gc_objects_lists[ECMA_GC_COLOR_BLACK]);
        ecma_gc_objects_lists[ECMA_GC_COLOR_BLACK] = obj_iter_p;

        if (likely (obj_prev_p != NULL))
        {
          JERRY_ASSERT (ecma_gc_get_object_next (obj_prev_p) == obj_iter_p);

          ecma_gc_set_object_next (obj_prev_p, obj_next_p);
        }
        else
        {
          ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY] = obj_next_p;
        }

        ecma_gc_mark (obj_iter_p);
        marked_anything_during_current_iteration = true;
      }
      else
      {
        obj_prev_p = obj_iter_p;
      }
    }
  }
  while (marked_anything_during_current_iteration);

  /* Sweeping objects that are currently unmarked */
  for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY], *obj_next_p;
       obj_iter_p != NULL;
       obj_iter_p = obj_next_p)
  {
    obj_next_p = ecma_gc_get_object_next (obj_iter_p);

    JERRY_ASSERT (!ecma_gc_is_object_visited (obj_iter_p));

    ecma_gc_sweep (obj_iter_p);
  }

  /* Unmarking all objects */
  ecma_gc_objects_lists[ECMA_GC_COLOR_WHITE_GRAY] = ecma_gc_objects_lists[ECMA_GC_COLOR_BLACK];
  ecma_gc_objects_lists[ECMA_GC_COLOR_BLACK] = NULL;

  ecma_gc_visited_flip_flag = !ecma_gc_visited_flip_flag;
} /* ecma_gc_run */

/**
 * Try to free some memory (depending on severity).
 */
void
ecma_try_to_give_back_some_memory (mem_try_give_memory_back_severity_t severity) /**< severity of
                                                                                  *   the request */
{
  if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW)
  {
    /*
     * If there is enough newly allocated objects since last GC, probably it is worthwhile to start GC now.
     * Otherwise, probability to free sufficient space is considered to be low.
     */
    if (ecma_gc_new_objects_since_last_gc * CONFIG_ECMA_GC_NEW_OBJECTS_SHARE_TO_START_GC > ecma_gc_objects_number)
    {
      ecma_gc_run ();
    }
  }
  else
  {
    JERRY_ASSERT (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);

    /* Freeing as much memory as we currently can */
    ecma_lcache_invalidate_all ();

    ecma_gc_run ();
  }
} /* ecma_try_to_give_back_some_memory */

/**
 * @}
 * @}
 */
