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

/**
 * Jerry printf implementation
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "jerry-libc-defs.h"

/**
 * printf's length type
 */
typedef enum
{
  LIBC_PRINTF_ARG_LENGTH_TYPE_NONE, /**< (none) */
  LIBC_PRINTF_ARG_LENGTH_TYPE_HH, /**< hh */
  LIBC_PRINTF_ARG_LENGTH_TYPE_H, /**< h */
  LIBC_PRINTF_ARG_LENGTH_TYPE_L, /**< l */
  LIBC_PRINTF_ARG_LENGTH_TYPE_LL, /**< ll */
  LIBC_PRINTF_ARG_LENGTH_TYPE_J, /**< j */
  LIBC_PRINTF_ARG_LENGTH_TYPE_Z, /**< z */
  LIBC_PRINTF_ARG_LENGTH_TYPE_T, /**< t */
  LIBC_PRINTF_ARG_LENGTH_TYPE_HIGHL /**< L */
} libc_printf_arg_length_type_t;

/**
 * printf's flags mask
 */
typedef uint8_t libc_printf_arg_flags_mask_t;

/**
 * Left justification of field's contents
 */
#define LIBC_PRINTF_ARG_FLAG_LEFT_JUSTIFY (1 << 0)

/**
 * Force print of number's sign
 */
#define LIBC_PRINTF_ARG_FLAG_PRINT_SIGN   (1 << 1)

/**
 * If no sign is printed, print space before value
 */
#define LIBC_PRINTF_ARG_FLAG_SPACE        (1 << 2)

/**
 * For o, x, X preceed value with 0, 0x or 0X for non-zero values.
 */
#define LIBC_PRINTF_ARG_FLAG_SHARP        (1 << 3)

/**
 * Left-pad field with zeroes instead of spaces
 */
#define LIBC_PRINTF_ARG_FLAG_ZERO_PADDING (1 << 4)

/**
 * printf helper function that outputs a char
 */
static void
libc_printf_putchar (FILE *stream, /**< stream pointer */
                     char character) /**< character */
{
  fwrite (&character, 1, sizeof (character), stream);
} /* libc_printf_putchar */

/**
 * printf helper function that outputs justified string
 */
static void
libc_printf_justified_string_output (FILE *stream, /**< stream pointer */
                                     const char *string_p, /**< string */
                                     size_t width, /**< minimum field width */
                                     bool is_left_justify, /**< justify to left (true) or right (false) */
                                     bool is_zero_padding) /**< left-pad with zeroes (true) or spaces (false) */
{
  const size_t str_length = strlen (string_p);

  size_t outputted_length = 0;

  if (!is_left_justify)
  {
    char padding_char = is_zero_padding ? '0' : ' ';

    while (outputted_length + str_length < width)
    {
      libc_printf_putchar (stream, padding_char);
      outputted_length++;
    }
  }

  fwrite (string_p, 1, str_length * sizeof (*string_p), stream);
  outputted_length += str_length;

  if (is_left_justify)
  {
    while (outputted_length < width)
    {
      libc_printf_putchar (stream, ' ');
      outputted_length++;
    }
  }
} /* libc_printf_justified_string_output */

/**
 * printf helper function that converts unsigned integer to string
 *
 * @return start of the string representation (within the output string buffer
 *         but not necessarily at its start)
 */
static char *
libc_printf_uint_to_string (uintmax_t value, /**< integer value */
                            char *buffer_p, /**< buffer for output string */
                            size_t buffer_size, /**< buffer size */
                            const char *alphabet, /**< alphabet used for digits */
                            uint32_t radix) /**< radix */
{
  char *str_buffer_end = buffer_p + buffer_size;
  char *str_p = str_buffer_end;
  *--str_p = '\0';

  assert (radix >= 2);

  if ((radix & (radix - 1)) != 0)
  {
    /*
     * Radix is not power of 2. Only 32-bit numbers are supported in this mode.
     */
    assert ((value >> 32) == 0);

    uint32_t value_lo = (uint32_t) value;

    while (value_lo != 0)
    {
      assert (str_p != buffer_p);

      *--str_p = alphabet[ value_lo % radix ];
      value_lo /= radix;
    }
  }
  else
  {
    uint32_t shift = 0;
    while (!(radix & (1u << shift)))
    {
      shift++;

      assert (shift <= 32);
    }

    uint32_t value_lo = (uint32_t) value;
    uint32_t value_hi = (uint32_t) (value >> 32);

    while (value_lo != 0
           || value_hi != 0)
    {
      assert (str_p != buffer_p);

      *--str_p = alphabet[ value_lo & (radix - 1) ];
      value_lo >>= shift;
      value_lo += (value_hi & (radix - 1)) << (32 - shift);
      value_hi >>= shift;
    }
  }

  if (*str_p == '\0')
  {
    *--str_p = '0';
  }

  assert (str_p >= buffer_p && str_p < str_buffer_end);

  return str_p;
} /* libc_printf_uint_to_string */

/**
 * printf helper function that prints d and i arguments
 *
 * @return updated va_list
 */
static void
libc_printf_write_d_i (FILE *stream, /**< stream pointer */
                       va_list *args_list_p, /**< args' list */
                       libc_printf_arg_flags_mask_t flags, /**< field's flags */
                       libc_printf_arg_length_type_t length, /**< field's length type */
                       uint32_t width) /**< minimum field width to output */
{
  assert ((flags & LIBC_PRINTF_ARG_FLAG_SHARP) == 0);

  bool is_signed = true;
  uintmax_t value = 0;

  /* true - positive, false - negative */
  bool sign = true;
  const size_t bits_in_byte = 8;
  const uintmax_t value_sign_mask = ((uintmax_t) 1) << (sizeof (value) * bits_in_byte - 1);

  switch (length)
  {
    case LIBC_PRINTF_ARG_LENGTH_TYPE_NONE:
    case LIBC_PRINTF_ARG_LENGTH_TYPE_HH: /* char is promoted to int */
    case LIBC_PRINTF_ARG_LENGTH_TYPE_H: /* short int is promoted to int */
    {
      value = (uintmax_t) va_arg (*args_list_p, int);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_L:
    {
      value = (uintmax_t) va_arg (*args_list_p, long int);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_LL:
    {
      value = (uintmax_t) va_arg (*args_list_p, long long int);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_J:
    {
      value = (uintmax_t) va_arg (*args_list_p, intmax_t);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_Z:
    {
      is_signed = false;
      value = (uintmax_t) va_arg (*args_list_p, size_t);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_T:
    {
      is_signed = false;
      value = (uintmax_t) va_arg (*args_list_p, ptrdiff_t);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_HIGHL:
    {
      assert (false && "unsupported length field L");
    }
  }

  if (is_signed)
  {
    sign = ((value & value_sign_mask) == 0);

    if (!sign)
    {
      value = (uintmax_t) (-value);
    }
  }

  char str_buffer[ 32 ];
  char *string_p = libc_printf_uint_to_string (value,
                                               str_buffer,
                                               sizeof (str_buffer),
                                               "0123456789",
                                               10);

  if (!sign
      || (flags & LIBC_PRINTF_ARG_FLAG_PRINT_SIGN))
  {
    assert (string_p > str_buffer);
    *--string_p = (sign ? '+' : '-');
  }
  else if (flags & LIBC_PRINTF_ARG_FLAG_SPACE)
  {
    /* no sign and space flag, printing one space */

    libc_printf_putchar (stream, ' ');
    if (width > 0)
    {
      width--;
    }
  }

  libc_printf_justified_string_output (stream,
                                       string_p,
                                       width,
                                       flags & LIBC_PRINTF_ARG_FLAG_LEFT_JUSTIFY,
                                       flags & LIBC_PRINTF_ARG_FLAG_ZERO_PADDING);
} /* libc_printf_write_d_i */

/**
 * printf helper function that prints d and i arguments
 *
 * @return updated va_list
 */
static void
libc_printf_write_u_o_x_X (FILE *stream, /**< stream pointer */
                           char specifier, /**< specifier (u, o, x, X) */
                           va_list *args_list_p, /**< args' list */
                           libc_printf_arg_flags_mask_t flags, /**< field's flags */
                           libc_printf_arg_length_type_t length, /**< field's length type */
                           uint32_t width) /**< minimum field width to output */
{
  uintmax_t value;

  switch (length)
  {
    case LIBC_PRINTF_ARG_LENGTH_TYPE_NONE:
    case LIBC_PRINTF_ARG_LENGTH_TYPE_HH: /* char is promoted to int */
    case LIBC_PRINTF_ARG_LENGTH_TYPE_H: /* short int is promoted to int */
    {
      value = (uintmax_t) va_arg (*args_list_p, unsigned int);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_L:
    {
      value = (uintmax_t) va_arg (*args_list_p, unsigned long int);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_LL:
    {
      value = (uintmax_t) va_arg (*args_list_p, unsigned long long int);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_J:
    {
      value = (uintmax_t) va_arg (*args_list_p, uintmax_t);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_Z:
    {
      value = (uintmax_t) va_arg (*args_list_p, size_t);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_T:
    {
      value = (uintmax_t) va_arg (*args_list_p, ptrdiff_t);
      break;
    }

    case LIBC_PRINTF_ARG_LENGTH_TYPE_HIGHL:
    {
      assert (false && "unsupported length field L");
      return;
    }

    default:
    {
      assert (false && "unexpected length field");
      return;
    }
  }

  if (flags & LIBC_PRINTF_ARG_FLAG_SHARP)
  {
    if (value != 0 && specifier != 'u')
    {
      libc_printf_putchar (stream, '0');

      if (specifier == 'x')
      {
        libc_printf_putchar (stream, 'x');
      }
      else if (specifier == 'X')
      {
        libc_printf_putchar (stream, 'X');
      }
      else
      {
        assert (specifier == 'o');
      }
    }
  }

  uint32_t radix;
  const char *alphabet;

  switch (specifier)
  {
    case 'u':
    {
      alphabet = "0123456789";
      radix = 10;
      break;
    }

    case 'o':
    {
      alphabet = "01234567";
      radix = 8;
      break;
    }

    case 'x':
    {
      alphabet = "0123456789abcdef";
      radix = 16;
      break;
    }

    case 'X':
    {
      alphabet = "0123456789ABCDEF";
      radix = 16;
      break;
    }

    default:
    {
      assert (false && "unexpected type field");
      return;
    }
  }

  char str_buffer[ 32 ];
  const char *string_p = libc_printf_uint_to_string (value,
                                                     str_buffer,
                                                     sizeof (str_buffer),
                                                     alphabet,
                                                     radix);

  if (flags & LIBC_PRINTF_ARG_FLAG_PRINT_SIGN)
  {
    /* printing sign */

    libc_printf_putchar (stream, '+');
    if (width > 0)
    {
      width--;
    }
  }
  else if (flags & LIBC_PRINTF_ARG_FLAG_SPACE)
  {
    /* no sign and space flag, printing one space */

    libc_printf_putchar (stream, ' ');
    if (width > 0)
    {
      width--;
    }
  }

  libc_printf_justified_string_output (stream,
                                       string_p,
                                       width,
                                       flags & LIBC_PRINTF_ARG_FLAG_LEFT_JUSTIFY,
                                       flags & LIBC_PRINTF_ARG_FLAG_ZERO_PADDING);
} /* libc_printf_write_u_o_x_X */

/**
 * vfprintf
 *
 * @return number of characters printed
 */
int __attr_weak___
vfprintf (FILE *stream, /**< stream pointer */
          const char *format, /**< format string */
          va_list args) /**< arguments */
{
  va_list args_copy;

  va_copy (args_copy, args);

  const char *format_iter_p = format;

  while (*format_iter_p)
  {
    if (*format_iter_p != '%')
    {
      libc_printf_putchar (stream, *format_iter_p);
    }
    else
    {
      libc_printf_arg_flags_mask_t flags = 0;
      uint32_t width = 0;
      libc_printf_arg_length_type_t length = LIBC_PRINTF_ARG_LENGTH_TYPE_NONE;

      while (true)
      {
        format_iter_p++;

        if (*format_iter_p == '-')
        {
          flags |= LIBC_PRINTF_ARG_FLAG_LEFT_JUSTIFY;
        }
        else if (*format_iter_p == '+')
        {
          flags |= LIBC_PRINTF_ARG_FLAG_PRINT_SIGN;
        }
        else if (*format_iter_p == ' ')
        {
          flags |= LIBC_PRINTF_ARG_FLAG_SPACE;
        }
        else if (*format_iter_p == '#')
        {
          flags |= LIBC_PRINTF_ARG_FLAG_SHARP;
        }
        else if (*format_iter_p == '0')
        {
          flags |= LIBC_PRINTF_ARG_FLAG_ZERO_PADDING;
        }
        else
        {
          break;
        }
      }

      if (*format_iter_p == '*')
      {
        assert (false && "unsupported width field *");
      }

      /* If there is a number, recognize it as field width. */
      while (*format_iter_p >= '0' && *format_iter_p <= '9')
      {
        width = width * 10u + (uint32_t) (*format_iter_p - '0');

        format_iter_p++;
      }

      if (*format_iter_p == '.')
      {
        assert (false && "unsupported precision field");
      }

      switch (*format_iter_p)
      {
        case 'h':
        {
          format_iter_p++;
          if (*format_iter_p == 'h')
          {
            format_iter_p++;

            length = LIBC_PRINTF_ARG_LENGTH_TYPE_HH;
          }
          else
          {
            length = LIBC_PRINTF_ARG_LENGTH_TYPE_H;
          }
          break;
        }

        case 'l':
        {
          format_iter_p++;
          if (*format_iter_p == 'l')
          {
            format_iter_p++;

            length = LIBC_PRINTF_ARG_LENGTH_TYPE_LL;
          }
          else
          {
            length = LIBC_PRINTF_ARG_LENGTH_TYPE_L;
          }
          break;
        }

        case 'j':
        {
          format_iter_p++;
          length = LIBC_PRINTF_ARG_LENGTH_TYPE_J;
          break;
        }

        case 'z':
        {
          format_iter_p++;
          length = LIBC_PRINTF_ARG_LENGTH_TYPE_Z;
          break;
        }

        case 't':
        {
          format_iter_p++;
          length = LIBC_PRINTF_ARG_LENGTH_TYPE_T;
          break;
        }

        case 'L':
        {
          format_iter_p++;
          length = LIBC_PRINTF_ARG_LENGTH_TYPE_HIGHL;
          break;
        }
      }

      switch (*format_iter_p)
      {
        case 'd':
        case 'i':
        {
          libc_printf_write_d_i (stream, &args_copy, flags, length, width);
          break;
        }

        case 'u':
        case 'o':
        case 'x':
        case 'X':
        {
          libc_printf_write_u_o_x_X (stream, *format_iter_p, &args_copy, flags, length, width);
          break;
        }

        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
        {
          assert (false && "unsupported double type field");
          break;
        }

        case 'c':
        {
          if (length & LIBC_PRINTF_ARG_LENGTH_TYPE_L)
          {
            assert (false && "unsupported length field L");
          }
          else
          {
            char str[2] =
            {
              (char) va_arg (args_copy, int), /* char is promoted to int */
              '\0'
            };

            libc_printf_justified_string_output (stream,
                                                 str,
                                                 width,
                                                 flags & LIBC_PRINTF_ARG_FLAG_LEFT_JUSTIFY,
                                                 flags & LIBC_PRINTF_ARG_FLAG_ZERO_PADDING);
          }
          break;
        }

        case 's':
        {
          if (length & LIBC_PRINTF_ARG_LENGTH_TYPE_L)
          {
            assert (false && "unsupported length field L");
          }
          else
          {
            char *str_p = va_arg (args_copy, char *);

            libc_printf_justified_string_output (stream,
                                                 str_p,
                                                 width,
                                                 flags & LIBC_PRINTF_ARG_FLAG_LEFT_JUSTIFY,
                                                 flags & LIBC_PRINTF_ARG_FLAG_ZERO_PADDING);
          }
          break;
        }

        case 'p':
        {
          va_list args_copy2;
          va_copy (args_copy2, args_copy);
          void *value = va_arg (args_copy2, void *);
          va_end (args_copy2);

          if (value == NULL)
          {
            printf ("(nil)");
          }
          else
          {
            libc_printf_write_u_o_x_X (stream,
                                       'x',
                                       &args_copy,
                                       flags | LIBC_PRINTF_ARG_FLAG_SHARP,
                                       LIBC_PRINTF_ARG_LENGTH_TYPE_Z,
                                       width);
          }
          break;
        }

        case 'n':
        {
          assert (false && "unsupported type field n");
        }
      }
    }

    format_iter_p++;
  }

  va_end (args_copy);

  return 0;
} /* vfprintf */

/**
 * fprintf
 *
 * @return number of characters printed
 */
int __attr_weak___
fprintf (FILE *stream, /**< stream pointer */
         const char *format, /**< format string */
         ...) /**< parameters' values */
{
  va_list args;

  va_start (args, format);

  int ret = vfprintf (stream, format, args);

  va_end (args);

  return ret;
} /* fprintf */

/**
 * printf
 *
 * @return number of characters printed
 */
int __attr_weak___
printf (const char *format, /**< format string */
        ...) /**< parameters' values */
{
  va_list args;

  va_start (args, format);

  int ret = vfprintf (stdout, format, args);

  va_end (args);

  return ret;
} /* printf */
