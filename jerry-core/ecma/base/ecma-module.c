/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "jcontext.h"
#include "jerryscript.h"

#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-module.h"
#include "ecma-objects.h"
#include "lit-char-helpers.h"
#include "vm.h"

#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)

/**
 * Takes a ModuleSpecifier and applies path normalization to it.
 * It's not checked if the ModuleSpecifier is a valid path or not.
 * Note: See 15.2.1.17
 *
 * @return pointer to ecma_string_t containing the normalized and zero terminated path
 */
ecma_string_t *
ecma_module_create_normalized_path (const uint8_t *char_p, /**< module specifier */
                                    prop_length_t size) /**< size of module specifier */
{
  JERRY_ASSERT (size > 0);
  ecma_string_t *ret_p = NULL;

  /* The module specifier is cesu8 encoded, we need to convert is to utf8, and zero terminate it,
   * so that OS level functions can handle it. */
  lit_utf8_byte_t *path_p = (lit_utf8_byte_t *) jmem_heap_alloc_block (size + 1u);

  lit_utf8_size_t utf8_size;
  utf8_size = lit_convert_cesu8_string_to_utf8_string (char_p,
                                                       size,
                                                       path_p,
                                                       size);
  path_p[utf8_size] = LIT_CHAR_NULL;

  lit_utf8_byte_t *module_path_p = NULL;
  lit_utf8_size_t module_path_size = 0;

  /* Check if we have a current module, and use its path as the base path. */
  JERRY_ASSERT (JERRY_CONTEXT (module_top_context_p) != NULL);
  if (JERRY_CONTEXT (module_top_context_p)->module_p != NULL)
  {
    JERRY_ASSERT (JERRY_CONTEXT (module_top_context_p)->module_p->path_p != NULL);
    module_path_size = ecma_string_get_size (JERRY_CONTEXT (module_top_context_p)->module_p->path_p);
    module_path_p = (lit_utf8_byte_t *) jmem_heap_alloc_block (module_path_size + 1);

    lit_utf8_size_t module_utf8_size;
    module_utf8_size = ecma_string_copy_to_utf8_buffer (JERRY_CONTEXT (module_top_context_p)->module_p->path_p,
                                                        module_path_p,
                                                        module_path_size);

    module_path_p[module_utf8_size] = LIT_CHAR_NULL;
  }

  lit_utf8_byte_t *normalized_out_p = (lit_utf8_byte_t *) jmem_heap_alloc_block (ECMA_MODULE_MAX_PATH);
  size_t normalized_size = jerry_port_normalize_path ((const char *) path_p,
                                                      (char *) normalized_out_p,
                                                      ECMA_MODULE_MAX_PATH,
                                                      (char *) module_path_p);


  if (normalized_size > 0)
  {
    /* Convert the normalized path to cesu8. */
    ret_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 (normalized_out_p, (lit_utf8_size_t) (normalized_size));
  }

  jmem_heap_free_block (path_p, size + 1u);
  jmem_heap_free_block (normalized_out_p, ECMA_MODULE_MAX_PATH);
  if (module_path_p != NULL)
  {
    jmem_heap_free_block (module_path_p, module_path_size + 1);
  }

  return ret_p;
} /* ecma_module_create_normalized_path */

/**
 * Find a module with a specific identifier
 *
 * @return pointer to ecma_module_t, if found
 *         NULL, otherwise
 */
ecma_module_t *
ecma_module_find_module (ecma_string_t *const path_p) /**< module identifier */
{
  ecma_module_t *current_p = JERRY_CONTEXT (ecma_modules_p);
  while (current_p != NULL)
  {
    if (ecma_compare_ecma_strings (path_p, current_p->path_p))
    {
      return current_p;
    }
    current_p = current_p->next_p;
  }

  return current_p;
} /* ecma_module_find_module */

/**
 * Create a new module
 *
 * @return pointer to created module
 */
static ecma_module_t *
ecma_module_create_module (ecma_string_t *const path_p) /**< module identifier */
{
  ecma_module_t *module_p = (ecma_module_t *) jmem_heap_alloc_block (sizeof (ecma_module_t));
  memset (module_p, 0, sizeof (ecma_module_t));

  module_p->path_p = path_p;
  module_p->next_p = JERRY_CONTEXT (ecma_modules_p);
  JERRY_CONTEXT (ecma_modules_p) = module_p;
  return module_p;
} /* ecma_module_create_module */

/**
 * Checks if we already have a module request in the module list.
 *
 * @return pointer to found or newly created module structure
 */
ecma_module_t *
ecma_module_find_or_create_module (ecma_string_t *const path_p) /**< module path */
{
  ecma_module_t *module_p = ecma_module_find_module (path_p);
  if (module_p)
  {
    ecma_deref_ecma_string (path_p);
    return module_p;
  }

  return ecma_module_create_module (path_p);
} /* ecma_module_find_or_create_module */

/**
 * Create a new native module
 *
 * @return pointer to created module
 */
ecma_module_t *
ecma_module_create_native_module (ecma_string_t *const path_p, /**< module identifier */
                                  ecma_object_t *const namespace_p) /**< module namespace */
{
  ecma_module_t *module_p = ecma_module_create_module (path_p);
  module_p->state = ECMA_MODULE_STATE_NATIVE;
  module_p->namespace_object_p = namespace_p;
  return module_p;
} /* ecma_module_create_native_module */

/**
 * Creates a module context.
 *
 * @return pointer to created module context
 */
static ecma_module_context_t *
ecma_module_create_module_context (void)
{
  ecma_module_context_t *context_p = (ecma_module_context_t *) jmem_heap_alloc_block (sizeof (ecma_module_context_t));
  memset (context_p, 0, sizeof (ecma_module_context_t));

  return context_p;
} /* ecma_module_create_module_context */

/**
 *  Inserts a {module, export_name} record into a resolve set.
 *  Note: See 15.2.1.16.3 - resolveSet and exportStarSet
 *
 *  @return true - if the set already contains the record
 *          false - otherwise
 */
bool
ecma_module_resolve_set_insert (ecma_module_resolve_set_t **set_p, /**< [in, out] resolve set */
                                ecma_module_t * const module_p, /**< module */
                                ecma_string_t * const export_name_p) /**< export name */
{
  JERRY_ASSERT (set_p != NULL);
  ecma_module_resolve_set_t *current_p = *set_p;

  while (current_p != NULL)
  {
    if (current_p->record.module_p == module_p
        && ecma_compare_ecma_strings (current_p->record.name_p, export_name_p))
    {
      return false;
    }

    current_p = current_p->next_p;
  }

  ecma_module_resolve_set_t *new_p;
  new_p = (ecma_module_resolve_set_t *) jmem_heap_alloc_block (sizeof (ecma_module_resolve_set_t));

  new_p->next_p = *set_p;
  new_p->record.module_p = module_p;
  ecma_ref_ecma_string (export_name_p);
  new_p->record.name_p = export_name_p;

  *set_p = new_p;
  return true;
} /* ecma_module_resolve_set_insert */

/**
 * Cleans up contents of a resolve set.
 */
void
ecma_module_resolve_set_cleanup (ecma_module_resolve_set_t *set_p) /**< resolve set */
{
  while (set_p != NULL)
  {
    ecma_module_resolve_set_t *next_p = set_p->next_p;
    ecma_deref_ecma_string (set_p->record.name_p);
    jmem_heap_free_block (set_p, sizeof (ecma_module_resolve_set_t));
    set_p = next_p;
  }
} /* ecma_module_resolve_set_cleanup */

/**
 * Pushes a new resolve frame on top of a resolve stack and initializes it
 * to begin resolving the specified exported name in the base module.
 */
void
ecma_module_resolve_stack_push (ecma_module_resolve_stack_t **stack_p, /**< [in, out] resolve stack */
                                ecma_module_t * const module_p, /**< base module */
                                ecma_string_t * const export_name_p) /**< exported name */
{
  JERRY_ASSERT (stack_p != NULL);
  ecma_module_resolve_stack_t *new_frame_p;
  new_frame_p = (ecma_module_resolve_stack_t *) jmem_heap_alloc_block (sizeof (ecma_module_resolve_stack_t));

  ecma_ref_ecma_string (export_name_p);
  new_frame_p->export_name_p = export_name_p;
  new_frame_p->module_p = module_p;
  new_frame_p->resolving = false;

  new_frame_p->next_p = *stack_p;
  *stack_p = new_frame_p;
} /* ecma_module_resolve_stack_push */

/**
 * Pops the topmost frame from a resolve stack.
 */
void
ecma_module_resolve_stack_pop (ecma_module_resolve_stack_t **stack_p) /**< [in, out] resolve stack */
{
  JERRY_ASSERT (stack_p != NULL);
  ecma_module_resolve_stack_t *current_p = *stack_p;

  if (current_p != NULL)
  {
    *stack_p = current_p->next_p;
    ecma_deref_ecma_string (current_p->export_name_p);
    jmem_heap_free_block (current_p, sizeof (ecma_module_resolve_stack_t));
  }
} /* ecma_module_resolve_stack_pop */

/**
 * Resolves which module satisfies an export based from a specific module in the import tree.
 * If no error occurs, out_record_p will contain a {module, local_name} record, which satisfies
 * the export, or {NULL, NULL} if the export is ambiguous.
 * Note: See 15.2.1.16.3
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
static ecma_value_t
ecma_module_resolve_export (ecma_module_t * const module_p, /**< base module */
                            ecma_string_t * const export_name_p, /**< export name */
                            ecma_module_record_t *out_record_p) /**< [out] found module record */
{
  ecma_module_resolve_set_t *resolve_set_p = NULL;
  ecma_module_resolve_stack_t *stack_p = NULL;

  bool found = false;
  ecma_module_record_t found_record = { NULL, NULL };
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ecma_module_resolve_stack_push (&stack_p, module_p, export_name_p);

  while (stack_p != NULL)
  {
    ecma_module_resolve_stack_t *current_frame_p = stack_p;

    ecma_module_t *current_module_p = current_frame_p->module_p;
    JERRY_ASSERT (current_module_p->state >= ECMA_MODULE_STATE_PARSED);
    ecma_module_context_t *context_p = current_module_p->context_p;
    ecma_string_t *current_export_name_p = current_frame_p->export_name_p;

    if (!current_frame_p->resolving)
    {
      current_frame_p->resolving = true;

      /* 15.2.1.16.3 / 2-3 */
      if (!ecma_module_resolve_set_insert (&resolve_set_p, current_module_p, current_export_name_p))
      {
        /* This is a circular import request. */
        ecma_module_resolve_stack_pop (&stack_p);
        continue;
      }

      if (current_module_p->state == ECMA_MODULE_STATE_NATIVE)
      {
        ecma_object_t *object_p = current_module_p->namespace_object_p;
        ecma_value_t prop_value = ecma_op_object_find_own (ecma_make_object_value (object_p),
                                                           object_p,
                                                           current_export_name_p);
        if (ecma_is_value_found (prop_value))
        {
          found = true;
          found_record.module_p = current_module_p;
          found_record.name_p = current_export_name_p;
          ecma_free_value (prop_value);
        }

        if (ecma_compare_ecma_string_to_magic_id (current_export_name_p, LIT_MAGIC_STRING_DEFAULT))
        {
          ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("No default export in native module."));
          break;
        }

        ecma_module_resolve_stack_pop (&stack_p);
        continue;
      }

      if (context_p->local_exports_p != NULL)
      {
        /* 15.2.1.16.3 / 4 */
        JERRY_ASSERT (context_p->local_exports_p->next_p == NULL);
        ecma_module_names_t *export_names_p = context_p->local_exports_p->module_names_p;
        while (export_names_p != NULL)
        {
          if (ecma_compare_ecma_strings (current_export_name_p, export_names_p->imex_name_p))
          {
            if (found)
            {
              /* This is an ambigous export. */
              found_record.module_p = NULL;
              found_record.name_p = NULL;
              break;
            }

            /* The current module provides a direct binding for this export. */
            found = true;
            found_record.module_p = current_module_p;
            found_record.name_p = export_names_p->local_name_p;
            break;
          }

          export_names_p = export_names_p->next_p;
        }
      }

      if (found)
      {
        /* We found a resolution for the current frame, return to the previous. */
        ecma_module_resolve_stack_pop (&stack_p);
        continue;
      }

      /* 15.2.1.16.3 / 5 */
      ecma_module_node_t *indirect_export_p = context_p->indirect_exports_p;
      while (indirect_export_p != NULL)
      {
        ecma_module_names_t *export_names_p = indirect_export_p->module_names_p;
        while (export_names_p != NULL)
        {
          if (ecma_compare_ecma_strings (current_export_name_p, export_names_p->imex_name_p))
          {
            /* 5.2.1.16.3 / 5.a.iv */
            ecma_module_resolve_stack_push (&stack_p,
                                            indirect_export_p->module_request_p,
                                            export_names_p->local_name_p);
          }

          export_names_p = export_names_p->next_p;
        }

        indirect_export_p = indirect_export_p->next_p;
      }

      /* We need to check whether the newly pushed indirect exports resolve to anything.
       * Keep current frame in the stack, and continue from the topmost frame. */
      continue;
    } /* if (!current_frame_p->resolving) */

    /* By the time we return to the current frame, the indirect exports will have finished resolving. */
    if (found)
    {
      /* We found at least one export that satisfies the current request.
       * Pop current frame, and return to the previous. */
      ecma_module_resolve_stack_pop (&stack_p);
      continue;
    }

    /* 15.2.1.16.3 / 6 */
    if (ecma_compare_ecma_string_to_magic_id (current_export_name_p, LIT_MAGIC_STRING_DEFAULT))
    {
      ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("No explicitly defined default export in module."));
      break;
    }

    /* 15.2.1.16.3 / 7-8 */
    if (!ecma_module_resolve_set_insert (&resolve_set_p,
                                         current_module_p,
                                         ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR)))
    {
      /* This is a circular import request. */
      ecma_module_resolve_stack_pop (&stack_p);
      continue;
    }

    /* Pop the current frame, we have nothing else to do here after the star export resolutions are queued. */
    ecma_module_resolve_stack_pop (&stack_p);

    /* 15.2.1.16.3 / 10 */
    ecma_module_node_t *star_export_p = context_p->star_exports_p;
    while (star_export_p != NULL)
    {
      JERRY_ASSERT (star_export_p->module_names_p == NULL);

      /* 15.2.1.16.3 / 10.c */
      ecma_module_resolve_stack_push (&stack_p, star_export_p->module_request_p, export_name_p);

      star_export_p = star_export_p->next_p;
    }
  }

  /* Clean up. */
  ecma_module_resolve_set_cleanup (resolve_set_p);
  while (stack_p)
  {
    ecma_module_resolve_stack_pop (&stack_p);
  }

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    /* No default export was found */
    return ret_value;
  }

  if (found)
  {
    *out_record_p = found_record;
  }
  else
  {
    ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Unexported or circular import request."));
  }

  return ret_value;
} /* ecma_module_resolve_export */

/**
 * Resolves an export and adds it to the modules namespace object, if the export name is not yet handled.
 * Note: See 15.2.1.16.2 and 15.2.1.18
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
static ecma_value_t
ecma_module_namespace_object_add_export_if_needed (ecma_module_t *module_p, /**< module */
                                                   ecma_string_t *export_name_p) /**< export name */
{
  JERRY_ASSERT (module_p->namespace_object_p != NULL);
  ecma_value_t result = ECMA_VALUE_EMPTY;

  if (ecma_find_named_property (module_p->namespace_object_p, export_name_p) != NULL)
  {
    /* This export name has already been handled. */
    return result;
  }

  ecma_module_record_t record;
  result = ecma_module_resolve_export (module_p, export_name_p, &record);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  if (record.module_p == NULL)
  {
    /* 15.2.1.18 / 3.d.iv Skip ambiguous names. */
    return result;
  }

  ecma_object_t *ref_base_lex_env_p;
  ecma_value_t prop_value = ecma_op_get_value_lex_env_base (record.module_p->scope_p,
                                                            &ref_base_lex_env_p,
                                                            record.name_p);
  ecma_property_t *new_property_p;
  ecma_create_named_data_property (module_p->namespace_object_p,
                                   export_name_p,
                                   ECMA_PROPERTY_FIXED,
                                   &new_property_p);

  ecma_named_data_property_assign_value (module_p->namespace_object_p,
                                         ECMA_PROPERTY_VALUE_PTR (new_property_p),
                                         prop_value);

  ecma_free_value (prop_value);
  return result;
} /* ecma_module_namespace_object_add_export_if_needed */

/**
 * Creates a namespace object for a module.
 * Note: See 15.2.1.18
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
static ecma_value_t
ecma_module_create_namespace_object (ecma_module_t *module_p) /**< module */
{
  ecma_value_t result = ECMA_VALUE_EMPTY;
  if (module_p->namespace_object_p != NULL)
  {
    return result;
  }

  JERRY_ASSERT (module_p->state == ECMA_MODULE_STATE_EVALUATED);
  ecma_module_resolve_set_t *resolve_set_p = NULL;
  ecma_module_resolve_stack_t *stack_p = NULL;

  module_p->namespace_object_p = ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE),
                                                     0,
                                                     ECMA_OBJECT_TYPE_GENERAL);

  ecma_module_resolve_stack_push (&stack_p, module_p, ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR));
  while (stack_p != NULL)
  {
    ecma_module_resolve_stack_t *current_frame_p = stack_p;
    ecma_module_t *current_module_p = current_frame_p->module_p;
    ecma_module_context_t *context_p = current_module_p->context_p;

    ecma_module_resolve_stack_pop (&stack_p);

    /* 15.2.1.16.2 / 2-3 */
    if (!ecma_module_resolve_set_insert (&resolve_set_p,
                                         current_module_p,
                                         ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR)))
    {
      /* Circular import. */
      continue;
    }

    if (context_p->local_exports_p != NULL)
    {
      /* 15.2.1.16.2 / 5 */
      JERRY_ASSERT (context_p->local_exports_p->next_p == NULL);
      ecma_module_names_t *export_names_p = context_p->local_exports_p->module_names_p;
      while (export_names_p != NULL && ecma_is_value_empty (result))
      {
        result = ecma_module_namespace_object_add_export_if_needed (module_p,
                                                                    export_names_p->imex_name_p);
        export_names_p = export_names_p->next_p;
      }
    }

    /* 15.2.1.16.2 / 6 */
    ecma_module_node_t *indirect_export_p = context_p->indirect_exports_p;
    while (indirect_export_p != NULL && ecma_is_value_empty (result))
    {
      ecma_module_names_t *export_names_p = indirect_export_p->module_names_p;
      while (export_names_p != NULL && ecma_is_value_empty (result))
      {
        result = ecma_module_namespace_object_add_export_if_needed (module_p,
                                                                    export_names_p->imex_name_p);
        export_names_p = export_names_p->next_p;
      }
      indirect_export_p = indirect_export_p->next_p;
    }

    /* 15.2.1.16.2 / 7 */
    ecma_module_node_t *star_export_p = context_p->star_exports_p;
    while (star_export_p != NULL && ecma_is_value_empty (result))
    {
      JERRY_ASSERT (star_export_p->module_names_p == NULL);

      /* 15.2.1.16.3/10.c */
      ecma_module_resolve_stack_push (&stack_p,
                                      star_export_p->module_request_p,
                                      ecma_get_magic_string (LIT_MAGIC_STRING_ASTERIX_CHAR));

      star_export_p = star_export_p->next_p;
    }
  }

  /* Clean up. */
  ecma_module_resolve_set_cleanup (resolve_set_p);
  while (stack_p)
  {
    ecma_module_resolve_stack_pop (&stack_p);
  }

  return result;
} /* ecma_module_create_namespace_object */

/**
 * Evaluates an EcmaScript module.
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
static ecma_value_t
ecma_module_evaluate (ecma_module_t *module_p) /**< module */
{
  JERRY_ASSERT (module_p->state >= ECMA_MODULE_STATE_PARSED);

  if (module_p->state >= ECMA_MODULE_STATE_EVALUATING)
  {
    return ECMA_VALUE_EMPTY;
  }

  module_p->state = ECMA_MODULE_STATE_EVALUATING;
  module_p->scope_p = ecma_create_decl_lex_env (ecma_get_global_environment ());
  module_p->context_p->parent_p = JERRY_CONTEXT (module_top_context_p);
  JERRY_CONTEXT (module_top_context_p) = module_p->context_p;

  ecma_value_t ret_value;
  ret_value = vm_run_module (module_p->compiled_code_p,
                             module_p->scope_p);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    jerry_release_value (ret_value);
    ret_value = ECMA_VALUE_EMPTY;
  }

  JERRY_CONTEXT (module_top_context_p) = module_p->context_p->parent_p;

  ecma_bytecode_deref (module_p->compiled_code_p);
  module_p->state = ECMA_MODULE_STATE_EVALUATED;

  return ret_value;
} /* ecma_module_evaluate */

/**
 * Connects imported values to the current context.
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
ecma_module_connect_imports (void)
{
  ecma_module_context_t *current_context_p = JERRY_CONTEXT (module_top_context_p);

  ecma_object_t *local_env_p = current_context_p->module_p->scope_p;
  JERRY_ASSERT (ecma_is_lexical_environment (local_env_p));

  ecma_module_node_t *import_node_p = current_context_p->imports_p;
  while (import_node_p != NULL)
  {
    ecma_value_t result = ecma_module_evaluate (import_node_p->module_request_p);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      return result;
    }

    ecma_module_names_t *import_names_p = import_node_p->module_names_p;
    while (import_names_p != NULL)
    {
      const bool is_namespace_import = ecma_compare_ecma_string_to_magic_id (import_names_p->imex_name_p,
                                                                             LIT_MAGIC_STRING_ASTERIX_CHAR);

      if (is_namespace_import)
      {
        result = ecma_module_create_namespace_object (import_node_p->module_request_p);
        if (ECMA_IS_VALUE_ERROR (result))
        {
          return result;
        }

        ecma_op_create_mutable_binding (local_env_p, import_names_p->local_name_p, true /* is_deletable */);
        ecma_op_set_mutable_binding (local_env_p,
                                     import_names_p->local_name_p,
                                     ecma_make_object_value (import_node_p->module_request_p->namespace_object_p),
                                     false /* is_strict */);
      }
      else /* !is_namespace_import */
      {
        ecma_module_record_t record;
        result = ecma_module_resolve_export (import_node_p->module_request_p, import_names_p->imex_name_p, &record);

        if (ECMA_IS_VALUE_ERROR (result))
        {
          return result;
        }

        if (record.module_p == NULL)
        {
          return ecma_raise_syntax_error (ECMA_ERR_MSG ("Ambiguous import request."));
        }

        if (record.module_p->state == ECMA_MODULE_STATE_NATIVE)
        {
          ecma_object_t *object_p = record.module_p->namespace_object_p;
          ecma_value_t prop_value = ecma_op_object_find_own (ecma_make_object_value (object_p),
                                                             object_p,
                                                             record.name_p);
          JERRY_ASSERT (ecma_is_value_found (prop_value));

          ecma_op_create_mutable_binding (local_env_p, import_names_p->local_name_p, true /* is_deletable */);
          ecma_op_set_mutable_binding (local_env_p,
                                       import_names_p->local_name_p,
                                       prop_value,
                                       false /* is_strict */);

          ecma_free_value (prop_value);
        }
        else
        {
          result = ecma_module_evaluate (record.module_p);

          if (ECMA_IS_VALUE_ERROR (result))
          {
            return result;
          }

          ecma_object_t *ref_base_lex_env_p;
          ecma_value_t prop_value = ecma_op_get_value_lex_env_base (record.module_p->scope_p,
                                                                    &ref_base_lex_env_p,
                                                                    record.name_p);

          ecma_op_create_mutable_binding (local_env_p, import_names_p->local_name_p, true /* is_deletable */);
          ecma_op_set_mutable_binding (local_env_p,
                                       import_names_p->local_name_p,
                                       prop_value,
                                       false /* is_strict */);

          ecma_free_value (prop_value);
        }
      }

      import_names_p = import_names_p->next_p;
    }

    import_node_p = import_node_p->next_p;
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_module_connect_imports */

/**
 * Parses an EcmaScript module.
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
static jerry_value_t
ecma_module_parse (ecma_module_t *module_p) /**< module */
{
  if (module_p->state >= ECMA_MODULE_STATE_PARSING)
  {
    return ECMA_VALUE_EMPTY;
  }

  module_p->state = ECMA_MODULE_STATE_PARSING;
  module_p->context_p = ecma_module_create_module_context ();

  lit_utf8_size_t module_path_size = ecma_string_get_size (module_p->path_p);
  lit_utf8_byte_t *module_path_p = (lit_utf8_byte_t *) jmem_heap_alloc_block (module_path_size + 1);

  lit_utf8_size_t module_path_utf8_size;
  module_path_utf8_size = ecma_string_copy_to_utf8_buffer (module_p->path_p,
                                                           module_path_p,
                                                           module_path_size);
  module_path_p[module_path_utf8_size] = LIT_CHAR_NULL;

  size_t source_size = 0;
  uint8_t *source_p = jerry_port_read_source ((const char *) module_path_p, &source_size);
  jmem_heap_free_block (module_path_p, module_path_size + 1);

  if (source_p == NULL)
  {
    return ecma_raise_syntax_error (ECMA_ERR_MSG ("File not found."));
  }

  module_p->context_p->module_p = module_p;
  module_p->context_p->parent_p = JERRY_CONTEXT (module_top_context_p);
  JERRY_CONTEXT (module_top_context_p) = module_p->context_p;

#if ENABLED (JERRY_DEBUGGER) && ENABLED (JERRY_PARSER)
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_string (JERRY_DEBUGGER_SOURCE_CODE_NAME,
                                JERRY_DEBUGGER_NO_SUBTYPE,
                                module_path_p,
                                module_path_size - 1);
  }
#endif /* ENABLED (JERRY_DEBUGGER) && ENABLED (JERRY_PARSER) */

  JERRY_CONTEXT (resource_name) = ecma_make_string_value (module_p->path_p);

  ecma_compiled_code_t *bytecode_data_p;
  ecma_value_t ret_value = parser_parse_script (NULL,
                                                0,
                                                (jerry_char_t *) source_p,
                                                source_size,
                                                ECMA_PARSE_STRICT_MODE | ECMA_PARSE_MODULE,
                                                &bytecode_data_p);

  JERRY_CONTEXT (module_top_context_p) = module_p->context_p->parent_p;

  jerry_port_release_source (source_p);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  ecma_free_value (ret_value);

  module_p->compiled_code_p = bytecode_data_p;
  module_p->state = ECMA_MODULE_STATE_PARSED;

  return ECMA_VALUE_EMPTY;
} /* ecma_module_parse */

/**
 * Parses all referenced modules.
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
ecma_module_parse_modules (void)
{
  ecma_module_t *current_p = JERRY_CONTEXT (ecma_modules_p);

  while (current_p != NULL)
  {
    ecma_value_t ret_value = ecma_module_parse (current_p);
    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      return ret_value;
    }

    JERRY_ASSERT (ecma_is_value_empty (ret_value));
    current_p = current_p->next_p;
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_module_parse_modules */

/**
 * Checks if indirect exports in the current context are resolvable.
 * Note: See 15.2.1.16.4 / 9.
 *
 * @return ECMA_VALUE_ERROR - if an error occured
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
ecma_module_check_indirect_exports (void)
{
  ecma_module_node_t *indirect_export_p = JERRY_CONTEXT (module_top_context_p)->indirect_exports_p;
  while (indirect_export_p != NULL)
  {
    ecma_module_names_t *name_p = indirect_export_p->module_names_p;
    while (name_p != NULL)
    {
      ecma_module_record_t record;
      ecma_value_t result = ecma_module_resolve_export (indirect_export_p->module_request_p,
                                                        name_p->local_name_p,
                                                        &record);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        return result;
      }

      if (record.module_p == NULL)
      {
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("Ambiguous indirect export request."));
      }

      name_p = name_p->next_p;
    }

    indirect_export_p = indirect_export_p->next_p;
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_module_check_indirect_exports */

/**
 * Cleans up a list of module names.
 */
static void
ecma_module_release_module_names (ecma_module_names_t *module_name_p) /**< first module name */
{
  while (module_name_p != NULL)
  {
    ecma_module_names_t *next_p = module_name_p->next_p;

    ecma_deref_ecma_string (module_name_p->imex_name_p);
    ecma_deref_ecma_string (module_name_p->local_name_p);
    jmem_heap_free_block (module_name_p, sizeof (ecma_module_names_t));

    module_name_p = next_p;
  }
} /* ecma_module_release_module_names */

/**
 * Cleans up a list of module nodes.
 */
void
ecma_module_release_module_nodes (ecma_module_node_t *module_node_p) /**< first module node */
{
  while (module_node_p != NULL)
  {
    ecma_module_node_t *next_p = module_node_p->next_p;

    ecma_module_release_module_names (module_node_p->module_names_p);
    jmem_heap_free_block (module_node_p, sizeof (ecma_module_node_t));

    module_node_p = next_p;
  }
} /* ecma_module_release_module_nodes */

/**
 * Cleans up a module context.
 */
static void
ecma_module_release_module_context (ecma_module_context_t *module_context_p) /**< modle context */
{
  ecma_module_release_module_nodes (module_context_p->imports_p);
  ecma_module_release_module_nodes (module_context_p->local_exports_p);
  ecma_module_release_module_nodes (module_context_p->indirect_exports_p);
  ecma_module_release_module_nodes (module_context_p->star_exports_p);

  jmem_heap_free_block (module_context_p, sizeof (ecma_module_context_t));
} /* ecma_module_release_module_context */

/**
 * Cleans up a module structure.
 */
static void
ecma_module_release_module (ecma_module_t *module_p) /**< module */
{
  ecma_deref_ecma_string (module_p->path_p);

  if (module_p->namespace_object_p != NULL)
  {
    ecma_deref_object (module_p->namespace_object_p);
  }

  if (module_p->state == ECMA_MODULE_STATE_NATIVE)
  {
    goto finished;
  }

  if (module_p->state >= ECMA_MODULE_STATE_PARSING)
  {
    ecma_module_release_module_context (module_p->context_p);
  }

  if (module_p->state >= ECMA_MODULE_STATE_EVALUATING)
  {
    ecma_deref_object (module_p->scope_p);
  }

  if (module_p->state >= ECMA_MODULE_STATE_PARSED
      && module_p->state < ECMA_MODULE_STATE_EVALUATED)
  {
    ecma_bytecode_deref (module_p->compiled_code_p);
  }

finished:
  jmem_heap_free_block (module_p, sizeof (ecma_module_t));
} /* ecma_module_release_module */

/**
 * Cleans up all modules if the current context is the root context.
 */
void
ecma_module_cleanup (void)
{
  if (JERRY_CONTEXT (module_top_context_p)->parent_p != NULL)
  {
    return;
  }

  ecma_module_t *current_p = JERRY_CONTEXT (ecma_modules_p);
  while (current_p != NULL)
  {
    ecma_module_t *next_p = current_p->next_p;
    ecma_module_release_module (current_p);
    current_p = next_p;
  }

  JERRY_CONTEXT (module_top_context_p) = NULL;
} /* ecma_module_cleanup */

#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
