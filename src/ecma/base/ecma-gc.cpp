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
#include "ecma-stack.h"
#include "jrt.h"
#include "jerry-libc.h"
#include "jrt-bit-fields.h"

/**
 * Global lists of objects sorted by generation identifier.
 */
static ecma_object_t *ecma_gc_objects_lists[ ECMA_GC_GEN_COUNT ];

static void ecma_gc_mark (ecma_object_t *object_p, ecma_gc_gen_t maximum_gen_to_traverse);
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
 * Get GC generation of the object.
 */
static ecma_gc_gen_t
ecma_gc_get_object_generation (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);

  ecma_gc_gen_t ret = (ecma_gc_gen_t) jrt_extract_bit_field (object_p->container,
                                                             ECMA_OBJECT_GC_GENERATION_POS,
                                                             ECMA_OBJECT_GC_GENERATION_WIDTH);

  JERRY_ASSERT (ret < ECMA_GC_GEN_COUNT);

  return ret;
} /* ecma_gc_get_object_generation */

/**
 * Set GC generation of the object.
 */
static void
ecma_gc_set_object_generation (ecma_object_t *object_p, /**< object */
                               ecma_gc_gen_t generation) /**< generation */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (generation < ECMA_GC_GEN_COUNT);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 generation,
                                                 ECMA_OBJECT_GC_GENERATION_POS,
                                                 ECMA_OBJECT_GC_GENERATION_WIDTH);
} /* ecma_gc_set_object_generation */

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

  return jrt_extract_bit_field (object_p->container,
                                ECMA_OBJECT_GC_VISITED_POS,
                                ECMA_OBJECT_GC_VISITED_WIDTH);
} /* ecma_gc_is_object_visited */

/**
 * Set visited flag of the object.
 */
static void
ecma_gc_set_object_visited (ecma_object_t *object_p, /**< object */
                            bool is_visited) /**< flag value */
{
  JERRY_ASSERT (object_p != NULL);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 is_visited,
                                                 ECMA_OBJECT_GC_VISITED_POS,
                                                 ECMA_OBJECT_GC_VISITED_WIDTH);
} /* ecma_gc_set_object_visited */

/**
 * Get may_ref_younger_objects flag of the object.
 */
static bool
ecma_gc_is_object_may_ref_younger_objects (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (object_p != NULL);

  return jrt_extract_bit_field (object_p->container,
                                ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_POS,
                                ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_WIDTH);
} /* ecma_gc_is_object_may_ref_younger_objects */

/**
 * Set may_ref_younger_objects flag of the object.
 */
static void
ecma_gc_set_object_may_ref_younger_objects (ecma_object_t *object_p, /**< object */
                                            bool is_may_ref_younger_objects) /**< flag value */
{
  JERRY_ASSERT (object_p != NULL);

  object_p->container = jrt_set_bit_field_value (object_p->container,
                                                 is_may_ref_younger_objects,
                                                 ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_POS,
                                                 ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_WIDTH);
} /* ecma_gc_set_object_may_ref_younger_objects */

/**
 * Initialize GC information for the object
 */
void
ecma_init_gc_info (ecma_object_t *object_p) /**< object */
{
  ecma_gc_set_object_refs (object_p, 1);

  ecma_gc_set_object_generation (object_p, ECMA_GC_GEN_0);
  ecma_gc_set_object_next (object_p, ecma_gc_objects_lists[ ECMA_GC_GEN_0 ]);
  ecma_gc_objects_lists[ ECMA_GC_GEN_0 ] = object_p;

  /* Should be set to false at the beginning of garbage collection */
  ecma_gc_set_object_visited (object_p, true);

  ecma_gc_set_object_may_ref_younger_objects (object_p, false);
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
  JERRY_ASSERT(ecma_gc_get_object_refs (object_p) > 0);
  ecma_gc_set_object_refs (object_p, ecma_gc_get_object_refs (object_p) - 1);
} /* ecma_deref_object */

/**
 * Set may_ref_younger_objects of specified object to true,
 * if value is object-value and it's object's generation
 * is less than generation of object specified by obj_p.
 */
void
ecma_gc_update_may_ref_younger_object_flag_by_value (ecma_object_t *obj_p, /**< object */
                                                     const ecma_value_t& value) /**< value */
{
  if (!ecma_is_value_object (value))
  {
    return;
  }

  ecma_object_t *ref_obj_p = ecma_get_object_from_value (value);
  JERRY_ASSERT(ref_obj_p != NULL);

  ecma_gc_update_may_ref_younger_object_flag_by_object (obj_p, ref_obj_p);
} /* ecma_gc_update_may_ref_younger_object_flag_by_value */

void
ecma_gc_update_may_ref_younger_object_flag_by_object (ecma_object_t *obj_p, /**< object */
                                                      ecma_object_t *ref_obj_p) /**< referenced object
                                                                                     or NULL */
{
  if (ref_obj_p == NULL)
  {
    return;
  }

  if (ecma_gc_get_object_generation (ref_obj_p) < ecma_gc_get_object_generation (obj_p))
  {
    ecma_gc_set_object_may_ref_younger_objects (obj_p, true);
  }
} /* ecma_gc_update_may_ref_younger_object_flag_by_object */

/**
 * Initialize garbage collector
 */
void
ecma_gc_init (void)
{
  __memset (ecma_gc_objects_lists, 0, sizeof (ecma_gc_objects_lists));
} /* ecma_gc_init */

/**
 * Mark objects as visited starting from specified object as root
 * if referenced object's generation is less or equal to maximum_gen_to_traverse.
 */
void
ecma_gc_mark (ecma_object_t *object_p, /**< start object */
              ecma_gc_gen_t maximum_gen_to_traverse) /**< start recursive traverse
                                                          if referenced object generation
                                                          is less or equal to maximum_gen_to_traverse */
{
  JERRY_ASSERT(object_p != NULL);

  ecma_gc_set_object_visited (object_p, true);

  bool does_ref_a_younger_object = false;
  bool traverse_properties = true;

  if (ecma_is_lexical_environment (object_p))
  {
    ecma_object_t *lex_env_p = ecma_get_lex_env_outer_reference (object_p);
    if (lex_env_p != NULL)
    {
      if (ecma_gc_get_object_generation (lex_env_p) <= maximum_gen_to_traverse)
      {
        if (!ecma_gc_is_object_visited (lex_env_p))
        {
          ecma_gc_mark (lex_env_p, ECMA_GC_GEN_COUNT);
        }
      }

      if (ecma_gc_get_object_generation (lex_env_p) < ecma_gc_get_object_generation (object_p))
      {
        does_ref_a_younger_object = true;
      }
    }

    if (ecma_get_lex_env_type (object_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND)
    {
      ecma_object_t *binding_object_p = ecma_get_lex_env_binding_object (object_p);
      if (ecma_gc_get_object_generation (binding_object_p) <= maximum_gen_to_traverse)
      {
        if (!ecma_gc_is_object_visited (binding_object_p))
        {
          ecma_gc_mark (binding_object_p, ECMA_GC_GEN_COUNT);
        }
      }

      if (ecma_gc_get_object_generation (binding_object_p) < ecma_gc_get_object_generation (object_p))
      {
        does_ref_a_younger_object = true;
      }

      traverse_properties = false;
    }
  }
  else
  {
    ecma_object_t *proto_p = ecma_get_object_prototype (object_p);
    if (proto_p != NULL)
    {
      if (ecma_gc_get_object_generation (proto_p) <= maximum_gen_to_traverse)
      {
        if (!ecma_gc_is_object_visited (proto_p))
        {
          ecma_gc_mark (proto_p, ECMA_GC_GEN_COUNT);
        }
      }

      if (ecma_gc_get_object_generation (proto_p) < ecma_gc_get_object_generation (object_p))
      {
        does_ref_a_younger_object = true;
      }
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

            if (ecma_gc_get_object_generation (value_obj_p) <= maximum_gen_to_traverse)
            {
              if (!ecma_gc_is_object_visited (value_obj_p))
              {
                ecma_gc_mark (value_obj_p, ECMA_GC_GEN_COUNT);
              }
            }

            if (ecma_gc_get_object_generation (value_obj_p) < ecma_gc_get_object_generation (object_p))
            {
              does_ref_a_younger_object = true;
            }
          }

          break;
        }

        case ECMA_PROPERTY_NAMEDACCESSOR:
        {
          ecma_object_t *getter_obj_p = ECMA_GET_POINTER (ecma_object_t,
                                                          property_p->u.named_accessor_property.get_p);
          ecma_object_t *setter_obj_p = ECMA_GET_POINTER (ecma_object_t,
                                                          property_p->u.named_accessor_property.set_p);

          if (getter_obj_p != NULL)
          {
            if (ecma_gc_get_object_generation (getter_obj_p) <= maximum_gen_to_traverse)
            {
              if (!ecma_gc_is_object_visited (getter_obj_p))
              {
                ecma_gc_mark (getter_obj_p, ECMA_GC_GEN_COUNT);
              }
            }

            if (ecma_gc_get_object_generation (getter_obj_p) < ecma_gc_get_object_generation (object_p))
            {
              does_ref_a_younger_object = true;
            }
          }

          if (setter_obj_p != NULL)
          {
            if (ecma_gc_get_object_generation (setter_obj_p) <= maximum_gen_to_traverse)
            {
              if (!ecma_gc_is_object_visited (setter_obj_p))
              {
                ecma_gc_mark (setter_obj_p, ECMA_GC_GEN_COUNT);
              }
            }

            if (ecma_gc_get_object_generation (setter_obj_p) < ecma_gc_get_object_generation (object_p))
            {
              does_ref_a_younger_object = true;
            }
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
              JERRY_UNIMPLEMENTED("Indexed array storage is not implemented yet.");
            }

            case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t
                                                        (see above in the routine) */
            case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t
                                                         (see above in the routine) */
            case ECMA_INTERNAL_PROPERTY__COUNT: /* not a real internal property type,
                                                 * but number of the real internal property types */
            {
              JERRY_UNREACHABLE();
            }

            case ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS: /* a collection of strings */
            case ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE: /* compressed pointer to a ecma_string_t */
            case ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE: /* compressed pointer to a ecma_number_t */
            case ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE: /* a simple boolean value */
            case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
            case ECMA_INTERNAL_PROPERTY_CODE: /* an integer */
            case ECMA_INTERNAL_PROPERTY_BUILT_IN_ID: /* an integer */
            case ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_ID: /* an integer */
            case ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31: /* an integer (bit-mask) */
            case ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63: /* an integer (bit-mask) */
            {
              break;
            }

            case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
            case ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP: /* an object */
            {
              ecma_object_t *obj_p = ECMA_GET_NON_NULL_POINTER(ecma_object_t, property_value);

              if (ecma_gc_get_object_generation (obj_p) <= maximum_gen_to_traverse)
              {
                if (!ecma_gc_is_object_visited (obj_p))
                {
                  ecma_gc_mark (obj_p, ECMA_GC_GEN_COUNT);
                }
              }

              if (ecma_gc_get_object_generation (obj_p) < ecma_gc_get_object_generation (object_p))
              {
                does_ref_a_younger_object = true;
              }

              break;
            }
          }

          break;
        }
      }
    }
  }

  if (!does_ref_a_younger_object)
  {
    ecma_gc_set_object_may_ref_younger_objects (object_p, false);
  }
} /* ecma_gc_mark */

/**
 * Free specified object
 */
void
ecma_gc_sweep (ecma_object_t *object_p) /**< object to free */
{
  JERRY_ASSERT(object_p != NULL
               && !ecma_gc_is_object_visited (object_p)
               && ecma_gc_get_object_refs (object_p) == 0);

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

  ecma_dealloc_object (object_p);
} /* ecma_gc_sweep */

/**
 * Run garbage collecting
 */
void
ecma_gc_run (ecma_gc_gen_t max_gen_to_collect) /**< maximum generation to run collection on */
{
  JERRY_ASSERT(max_gen_to_collect < ECMA_GC_GEN_COUNT);

  /* clearing visited flags for all objects of generations to be processed */
  for (ecma_gc_gen_t gen_id = ECMA_GC_GEN_0; gen_id <= max_gen_to_collect; gen_id = (ecma_gc_gen_t) (gen_id + 1))
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ecma_gc_get_object_next (obj_iter_p))
    {
      ecma_gc_set_object_visited (obj_iter_p, false);
    }
  }

  /* if some object is referenced from stack or globals (i.e. it is root),
   * start recursive marking traverse from the object */
  for (ecma_gc_gen_t gen_id = ECMA_GC_GEN_0; gen_id <= max_gen_to_collect; gen_id = (ecma_gc_gen_t) (gen_id + 1))
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ecma_gc_get_object_next (obj_iter_p))
    {
      if (ecma_gc_get_object_refs (obj_iter_p) > 0
          && !ecma_gc_is_object_visited (obj_iter_p))
      {
        ecma_gc_mark (obj_iter_p, ECMA_GC_GEN_COUNT);
      }
    }
  }

  /* if some object is referenced from a register variable (i.e. it is root),
   * start recursive marking traverse from the object */
  for (ecma_stack_frame_t *frame_iter_p = ecma_stack_get_top_frame ();
       frame_iter_p != NULL;
       frame_iter_p = frame_iter_p->prev_frame_p)
  {
    for (int32_t reg_index = 0; reg_index < frame_iter_p->regs_number; reg_index++)
    {
      ecma_value_t reg_value = ecma_stack_frame_get_reg_value (frame_iter_p, reg_index);

      if (ecma_is_value_object (reg_value))
      {
        ecma_object_t *obj_p = ecma_get_object_from_value (reg_value);

        if (!ecma_gc_is_object_visited (obj_p))
        {
          ecma_gc_mark (obj_p, ECMA_GC_GEN_COUNT);
        }
      }
    }
  }

  /* if some object from generations that are not processed during current session may reference
   * younger generations, start recursive marking traverse from the object, but one the first level
   * consider only references to object of at most max_gen_to_collect generation */
  for (ecma_gc_gen_t gen_id = (ecma_gc_gen_t) (max_gen_to_collect + 1);
       gen_id < ECMA_GC_GEN_COUNT;
       gen_id = (ecma_gc_gen_t) (gen_id + 1))
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ecma_gc_get_object_next (obj_iter_p))
    {
      if (ecma_gc_is_object_may_ref_younger_objects (obj_iter_p))
      {
        ecma_gc_mark (obj_iter_p, max_gen_to_collect);
      }
#ifndef JERRY_NDEBUG
      else if (gen_id > ECMA_GC_GEN_0)
      {
        ecma_gc_set_object_may_ref_younger_objects (obj_iter_p, true);
        ecma_gc_mark (obj_iter_p, (ecma_gc_gen_t) (gen_id - 1));
        JERRY_ASSERT (!ecma_gc_is_object_may_ref_younger_objects (obj_iter_p));
      }
#endif /* !JERRY_NDEBUG */
    }
  }

  JERRY_ASSERT (max_gen_to_collect <= ECMA_GC_GEN_COUNT);
  ecma_object_t *gen_last_obj_p[ ECMA_GC_GEN_COUNT ];
#ifndef JERRY_NDEBUG
  __memset (gen_last_obj_p, 0, sizeof (gen_last_obj_p));
#endif /* !JERRY_NDEBUG */

  for (ecma_gc_gen_t gen_id = ECMA_GC_GEN_0; gen_id <= max_gen_to_collect; gen_id = (ecma_gc_gen_t) (gen_id + 1))
  {
    ecma_object_t *obj_prev_p = NULL;

    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ],
         *obj_next_p;
         obj_iter_p != NULL;
         obj_iter_p = obj_next_p)
    {
      obj_next_p = ecma_gc_get_object_next (obj_iter_p);

      if (!ecma_gc_is_object_visited (obj_iter_p))
      {
        ecma_gc_sweep (obj_iter_p);

        if (likely (obj_prev_p != NULL))
        {
          ecma_gc_set_object_next (obj_prev_p, obj_next_p);
        }
        else
        {
          ecma_gc_objects_lists[ gen_id ] = obj_next_p;
        }
      }
      else
      {
        obj_prev_p = obj_iter_p;

        if (ecma_gc_get_object_generation (obj_iter_p) != ECMA_GC_GEN_COUNT - 1)
        {
          /* the object will be promoted to next generation */
          ecma_gc_set_object_generation (obj_iter_p, (ecma_gc_gen_t) (ecma_gc_get_object_generation (obj_iter_p) + 1));
        }
      }
    }

    gen_last_obj_p[ gen_id ] = obj_prev_p;
  }

  ecma_gc_gen_t gen_to_promote = max_gen_to_collect;
  if (unlikely (gen_to_promote == ECMA_GC_GEN_COUNT - 1))
  {
    /* not promoting last generation */
    gen_to_promote = (ecma_gc_gen_t) (gen_to_promote - 1);
  }

  /* promoting to next generation */
  if (gen_last_obj_p[ gen_to_promote ] != NULL)
  {
    ecma_gc_set_object_next (gen_last_obj_p [gen_to_promote], ecma_gc_objects_lists[ gen_to_promote + 1 ]);
    ecma_gc_objects_lists[ gen_to_promote + 1 ] = ecma_gc_objects_lists[ gen_to_promote ];
    ecma_gc_objects_lists[ gen_to_promote ] = NULL;
  }

  for (int32_t gen_id = (int32_t)gen_to_promote - 1;
       gen_id >= 0;
       gen_id--)
  {
    ecma_gc_objects_lists[ gen_id + 1 ] = ecma_gc_objects_lists[ gen_id ];
    ecma_gc_objects_lists[ gen_id ] = NULL;
  }

#ifndef JERRY_NDEBUG
  for (ecma_gc_gen_t gen_id = ECMA_GC_GEN_0;
       gen_id < ECMA_GC_GEN_COUNT;
       gen_id = (ecma_gc_gen_t) (gen_id + 1))
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ecma_gc_get_object_next (obj_iter_p))
    {
      JERRY_ASSERT(ecma_gc_get_object_generation (obj_iter_p) == gen_id);
    }
  }
#endif /* !JERRY_NDEBUG */
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
    ecma_gc_run (ECMA_GC_GEN_0);
  }
  else if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_MEDIUM)
  {
    ecma_gc_run (ECMA_GC_GEN_1);
  }
  else if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH)
  {
    ecma_gc_run (ECMA_GC_GEN_2);
  }
  else
  {
    JERRY_ASSERT (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_CRITICAL);

    /* Freeing as much memory as we currently can */
    ecma_lcache_invalidate_all ();

    ecma_gc_run ((ecma_gc_gen_t) (ECMA_GC_GEN_COUNT - 1));
  }
} /* ecma_try_to_give_back_some_memory */

/**
 * @}
 * @}
 */
