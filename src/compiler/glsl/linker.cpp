/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file linker.cpp
 * GLSL linker implementation
 *
 * Given a set of shaders that are to be linked to generate a final program,
 * there are three distinct stages.
 *
 * In the first stage shaders are partitioned into groups based on the shader
 * type.  All shaders of a particular type (e.g., vertex shaders) are linked
 * together.
 *
 *   - Undefined references in each shader are resolve to definitions in
 *     another shader.
 *   - Types and qualifiers of uniforms, outputs, and global variables defined
 *     in multiple shaders with the same name are verified to be the same.
 *   - Initializers for uniforms and global variables defined
 *     in multiple shaders with the same name are verified to be the same.
 *
 * The result, in the terminology of the GLSL spec, is a set of shader
 * executables for each processing unit.
 *
 * After the first stage is complete, a series of semantic checks are performed
 * on each of the shader executables.
 *
 *   - Each shader executable must define a \c main function.
 *   - Each vertex shader executable must write to \c gl_Position.
 *   - Each fragment shader executable must write to either \c gl_FragData or
 *     \c gl_FragColor.
 *
 * In the final stage individual shader executables are linked to create a
 * complete exectuable.
 *
 *   - Types of uniforms defined in multiple shader stages with the same name
 *     are verified to be the same.
 *   - Initializers for uniforms defined in multiple shader stages with the
 *     same name are verified to be the same.
 *   - Types and qualifiers of outputs defined in one stage are verified to
 *     be the same as the types and qualifiers of inputs defined with the same
 *     name in a later stage.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include <ctype.h>
#include "util/strndup.h"
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "ir.h"
#include "nir.h"
#include "program.h"
#include "program/prog_instruction.h"
#include "program/program.h"
#include "util/mesa-sha1.h"
#include "util/set.h"
#include "string_to_uint_map.h"
#include "linker_util.h"
#include "ir_optimization.h"
#include "ir_rvalue_visitor.h"
#include "ir_uniform.h"
#include "builtin_functions.h"
#include "shader_cache.h"
#include "util/u_string.h"
#include "util/u_math.h"


#include "main/shaderobj.h"
#include "main/enums.h"
#include "main/mtypes.h"
#include "main/context.h"


void
linker_error(gl_shader_program *prog, const char *fmt, ...)
{
   va_list ap;

   ralloc_strcat(&prog->data->InfoLog, "error: ");
   va_start(ap, fmt);
   ralloc_vasprintf_append(&prog->data->InfoLog, fmt, ap);
   va_end(ap);

   prog->data->LinkStatus = LINKING_FAILURE;
}


void
linker_warning(gl_shader_program *prog, const char *fmt, ...)
{
   va_list ap;

   ralloc_strcat(&prog->data->InfoLog, "warning: ");
   va_start(ap, fmt);
   ralloc_vasprintf_append(&prog->data->InfoLog, fmt, ap);
   va_end(ap);

}

void
link_shaders(struct gl_context *ctx, struct gl_shader_program *prog)
{
   prog->data->LinkStatus = LINKING_SUCCESS; /* All error paths will set this to false */
   prog->data->Validated = false;

   /* Section 7.3 (Program Objects) of the OpenGL 4.5 Core Profile spec says:
    *
    *     "Linking can fail for a variety of reasons as specified in the
    *     OpenGL Shading Language Specification, as well as any of the
    *     following reasons:
    *
    *     - No shader objects are attached to program."
    *
    * The Compatibility Profile specification does not list the error.  In
    * Compatibility Profile missing shader stages are replaced by
    * fixed-function.  This applies to the case where all stages are
    * missing.
    */
   if (prog->NumShaders == 0) {
      if (ctx->API != API_OPENGL_COMPAT)
         linker_error(prog, "no shaders attached to the program\n");
      return;
   }

#ifdef ENABLE_SHADER_CACHE
   if (shader_cache_read_program_metadata(ctx, prog))
      return;
#endif
}

void
resource_name_updated(struct gl_resource_name *name)
{
   if (name->string) {
      name->length = strlen(name->string);

      const char *last_square_bracket = strrchr(name->string, '[');
      if (last_square_bracket) {
         name->last_square_bracket = last_square_bracket - name->string;
         name->suffix_is_zero_square_bracketed =
            strcmp(last_square_bracket, "[0]") == 0;
      } else {
         name->last_square_bracket = -1;
         name->suffix_is_zero_square_bracketed = false;
      }
   } else {
      name->length = 0;
      name->last_square_bracket = -1;
      name->suffix_is_zero_square_bracketed = false;
   }
}
