/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/** \addtogroup ecma ---TODO---
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
#include "globals.h"
#include "jerry-libc.h"

/**
 * Global lists of objects sorted by generation identifier.
 */
static ecma_object_t *ecma_gc_objects_lists[ ECMA_GC_GEN_COUNT ];

static void ecma_gc_mark (ecma_object_t *object_p, ecma_gc_gen_t maximum_gen_to_traverse);
static void ecma_gc_sweep (ecma_object_t *object_p);

/**
 * Initialize GC information for the object
 */
void
ecma_init_gc_info (ecma_object_t *object_p) /**< object */
{
  object_p->gc_info.refs = 1;

  object_p->gc_info.generation = ECMA_GC_GEN_0;
  ECMA_SET_POINTER(object_p->gc_info.next, ecma_gc_objects_lists[ ECMA_GC_GEN_0 ]);
  ecma_gc_objects_lists[ ECMA_GC_GEN_0 ] = object_p;

  /* Should be set to false at the beginning of garbage collection */
  object_p->gc_info.visited = true;

  object_p->gc_info.may_ref_younger_objects = false;
} /* ecma_init_gc_info */

/**
 * Increase reference counter of an object
 */
void
ecma_ref_object (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT(object_p != NULL);
  object_p->gc_info.refs++;

  /**
   * Check that value was not overflowed
   */
  JERRY_ASSERT(object_p->gc_info.refs > 0);

  if (unlikely (object_p->gc_info.refs == 0))
  {
    JERRY_UNREACHABLE();
  }
} /* ecma_ref_object */

/**
 * Decrease reference counter of an object
 */
void
ecma_deref_object (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT(object_p != NULL);
  JERRY_ASSERT(object_p->gc_info.refs > 0);

  object_p->gc_info.refs--;
} /* ecma_deref_object */

/**
 * Set may_ref_younger_objects of specified object to true,
 * if value is object-value and it's object's generation
 * is less than generation of object specified by obj_p.
 */
void
ecma_gc_update_may_ref_younger_object_flag_by_value (ecma_object_t *obj_p, /**< object */
                                                     ecma_value_t value) /**< value */
{
  if (value.value_type != ECMA_TYPE_OBJECT)
  {
    return;
  }

  ecma_object_t *ref_obj_p = ECMA_GET_POINTER(value.value);
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

  if (ref_obj_p->gc_info.generation < obj_p->gc_info.generation)
  {
    obj_p->gc_info.may_ref_younger_objects = true;
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

  object_p->gc_info.visited = true;

  bool does_reference_object_to_traverse = false;

  if (object_p->is_lexical_environment)
  {
    ecma_object_t *lex_env_p = ECMA_GET_POINTER(object_p->u.lexical_environment.outer_reference_p);
    if (lex_env_p != NULL
        && lex_env_p->gc_info.generation <= maximum_gen_to_traverse)
    {
      if (!lex_env_p->gc_info.visited)
      {
        ecma_gc_mark (lex_env_p, ECMA_GC_GEN_COUNT);
      }

      does_reference_object_to_traverse = true;
    }
  }
  else
  {
    ecma_object_t *proto_p = ECMA_GET_POINTER(object_p->u.object.prototype_object_p);
    if (proto_p != NULL
        && proto_p->gc_info.generation <= maximum_gen_to_traverse)
    {
      if (!proto_p->gc_info.visited)
      {
        ecma_gc_mark (proto_p, ECMA_GC_GEN_COUNT);
      }

      does_reference_object_to_traverse = true;
    }
  }

  for (ecma_property_t *property_p = ECMA_GET_POINTER(object_p->properties_p), *next_property_p;
       property_p != NULL;
       property_p = next_property_p)
  {
    next_property_p = ECMA_GET_POINTER(property_p->next_property_p);

    switch ((ecma_property_type_t) property_p->type)
    {
      case ECMA_PROPERTY_NAMEDDATA:
      {
        ecma_value_t value = property_p->u.named_data_property.value;

        if (value.value_type == ECMA_TYPE_OBJECT)
        {
          ecma_object_t *value_obj_p = ECMA_GET_POINTER(value.value);

          if (value_obj_p->gc_info.generation <= maximum_gen_to_traverse)
          {
            if (!value_obj_p->gc_info.visited)
            {
              ecma_gc_mark (value_obj_p, ECMA_GC_GEN_COUNT);
            }

            does_reference_object_to_traverse = true;
          }
        }

        break;
      }

      case ECMA_PROPERTY_NAMEDACCESSOR:
      {
        ecma_object_t *getter_obj_p = ECMA_GET_POINTER(property_p->u.named_accessor_property.get_p);
        ecma_object_t *setter_obj_p = ECMA_GET_POINTER(property_p->u.named_accessor_property.set_p);

        if (getter_obj_p != NULL)
        {
          if (getter_obj_p->gc_info.generation <= maximum_gen_to_traverse)
          {
            if (!getter_obj_p->gc_info.visited)
            {
              ecma_gc_mark (getter_obj_p, ECMA_GC_GEN_COUNT);
            }

            does_reference_object_to_traverse = true;
          }
        }

        if (setter_obj_p != NULL)
        {
          if (setter_obj_p->gc_info.generation <= maximum_gen_to_traverse)
          {
            if (!setter_obj_p->gc_info.visited)
            {
              ecma_gc_mark (setter_obj_p, ECMA_GC_GEN_COUNT);
            }

            does_reference_object_to_traverse = true;
          }
        }

        break;
      }

      case ECMA_PROPERTY_INTERNAL:
      {
        ecma_internal_property_id_t property_id = property_p->u.internal_property.type;
        uint32_t property_value = property_p->u.internal_property.value;

        switch (property_id)
        {
          case ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES: /* a collection of ecma-values */
          case ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES: /* a collection of ecma-values */
          {
            JERRY_UNIMPLEMENTED();
          }

          case ECMA_INTERNAL_PROPERTY_PROTOTYPE: /* the property's value is located in ecma_object_t
                                                    (see above in the routine) */
          case ECMA_INTERNAL_PROPERTY_EXTENSIBLE: /* the property's value is located in ecma_object_t
                                                     (see above in the routine) */
          {
            JERRY_UNREACHABLE();
          }

          case ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS: /* a collection of strings */
          case ECMA_INTERNAL_PROPERTY_PROVIDE_THIS: /* a boolean */
          case ECMA_INTERNAL_PROPERTY_CLASS: /* an enum */
          case ECMA_INTERNAL_PROPERTY_CODE: /* an integer */
          {
            break;
          }

          case ECMA_INTERNAL_PROPERTY_SCOPE: /* a lexical environment */
          case ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP: /* an object */
          case ECMA_INTERNAL_PROPERTY_BINDING_OBJECT: /* an object */
          {
            ecma_object_t *obj_p = ECMA_GET_POINTER(property_value);

            if (obj_p->gc_info.generation <= maximum_gen_to_traverse)
            {
              if (!obj_p->gc_info.visited)
              {
                ecma_gc_mark (obj_p, ECMA_GC_GEN_COUNT);
              }

              does_reference_object_to_traverse = true;
            }

            break;
          }
        }

        break;
      }
    }
  }

  if (!does_reference_object_to_traverse)
  {
    object_p->gc_info.may_ref_younger_objects = false;
  }
} /* ecma_gc_mark */

/**
 * Free specified object
 */
void
ecma_gc_sweep (ecma_object_t *object_p) /**< object to free */
{
  JERRY_ASSERT(object_p != NULL
               && !object_p->gc_info.visited
               && object_p->gc_info.refs == 0);

  for (ecma_property_t *property = ECMA_GET_POINTER(object_p->properties_p),
       *next_property_p;
       property != NULL;
       property = next_property_p)
  {
    next_property_p = ECMA_GET_POINTER(property->next_property_p);

    ecma_free_property (property);
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
  for (ecma_gc_gen_t gen_id = 0; gen_id <= max_gen_to_collect; gen_id++)
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ECMA_GET_POINTER(obj_iter_p->gc_info.next))
    {
      obj_iter_p->gc_info.visited = false;
    }
  }

  /* if some object is referenced from stack or globals (i.e. it is root),
   * start recursive marking traverse from the object */
  for (ecma_gc_gen_t gen_id = 0; gen_id <= max_gen_to_collect; gen_id++)
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ECMA_GET_POINTER(obj_iter_p->gc_info.next))
    {
      if (obj_iter_p->gc_info.refs > 0
          && !obj_iter_p->gc_info.visited)
      {
        ecma_gc_mark (obj_iter_p, ECMA_GC_GEN_COUNT);
      }
    }
  }

  /* if some object from generations that are not processed during current session may reference
   * younger generations, start recursive marking traverse from the object, but one the first level
   * consider only references to object of at most max_gen_to_collect generation */
  for (ecma_gc_gen_t gen_id = max_gen_to_collect + 1; gen_id < ECMA_GC_GEN_COUNT; gen_id++)
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ECMA_GET_POINTER(obj_iter_p->gc_info.next))
    {
      if (obj_iter_p->gc_info.may_ref_younger_objects > 0)
      {
        ecma_gc_mark (obj_iter_p, max_gen_to_collect);
      }
    }
  }

  ecma_object_t *gen_last_obj_p[ max_gen_to_collect + 1 ];
#ifndef JERRY_NDEBUG
  __memset (gen_last_obj_p, 0, sizeof (gen_last_obj_p));
#endif /* !JERRY_NDEBUG */

  for (ecma_gc_gen_t gen_id = 0; gen_id <= max_gen_to_collect; gen_id++)
  {
    ecma_object_t *obj_prev_p = NULL;

    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ],
         *obj_next_p;
         obj_iter_p != NULL;
         obj_iter_p = obj_next_p)
    {
      obj_next_p = ECMA_GET_POINTER(obj_iter_p->gc_info.next);

      if (!obj_iter_p->gc_info.visited)
      {
        ecma_gc_sweep (obj_iter_p);

        if (likely (obj_prev_p != NULL))
        {
          ECMA_SET_POINTER(obj_prev_p->gc_info.next, obj_next_p);
        }
        else
        {
          ecma_gc_objects_lists[ gen_id ] = obj_next_p;
        }
      }
      else
      {
        obj_prev_p = obj_iter_p;

        if (obj_iter_p->gc_info.generation != ECMA_GC_GEN_COUNT - 1)
        {
          /* the object will be promoted to next generation */
          obj_iter_p->gc_info.generation++;
        }
      }
    }

    gen_last_obj_p[ gen_id ] = obj_prev_p;
  }

  ecma_gc_gen_t gen_to_promote = max_gen_to_collect;
  if (unlikely (gen_to_promote == ECMA_GC_GEN_COUNT - 1))
  {
    /* not promoting last generation */
    gen_to_promote--;
  }

  /* promoting to next generation */
  if (gen_last_obj_p[ gen_to_promote ] != NULL)
  {
    ECMA_SET_POINTER(gen_last_obj_p[ gen_to_promote ]->gc_info.next, ecma_gc_objects_lists[ gen_to_promote + 1 ]);
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
       gen_id++)
  {
    for (ecma_object_t *obj_iter_p = ecma_gc_objects_lists[ gen_id ];
         obj_iter_p != NULL;
         obj_iter_p = ECMA_GET_POINTER(obj_iter_p->gc_info.next))
    {
      JERRY_ASSERT(obj_iter_p->gc_info.generation == gen_id);
    }
  }
#endif /* !JERRY_NDEBUG */
} /* ecma_gc_run */

/**
 * @}
 * @}
 */
