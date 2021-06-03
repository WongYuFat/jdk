/*
 * Copyright (c) 2021, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2021, Datadog, Inc. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "code/codeBlob.hpp"
#include "code/compiledMethod.hpp"
#include "interpreter/interpreter.hpp"
#include "runtime/frame.hpp"

bool CodeBlob::FrameParser::sender_frame(JavaThread *thread, address pc, intptr_t* sp, intptr_t* unextended_sp, intptr_t* fp, bool fp_safe,
    address* sender_pc, intptr_t** sender_sp, intptr_t** sender_unextended_sp, intptr_t** sender_fp) {
  // must be some sort of compiled/runtime frame
  // fp does not have to be safe (although it could be check for c1?)

  assert(sender_pc != NULL, "invariant");
  assert(sender_sp != NULL, "invariant");

  // check for a valid frame_size, otherwise we are unlikely to get a valid sender_pc
  if (_cb->frame_size() <= 0) {
    return false;
  }

  *sender_sp = (intptr_t*)(unextended_sp + _cb->frame_size());
  // Is sender_sp safe?
  if (!thread->is_in_full_stack_checked((address)*sender_sp)) {
    return false;
  }
  // On Intel the return_address is always the word on the stack
  *sender_pc = (address) (*sender_sp)[-1];
  if (sender_unextended_sp) *sender_unextended_sp = *sender_sp;
  // Note: frame::sender_sp_offset is only valid for compiled frame
  if (sender_fp) *sender_fp = (intptr_t*) (*sender_sp)[-frame::sender_sp_offset];

  return true;
}

bool InterpreterBlob::FrameParser::sender_frame(JavaThread *thread, address pc, intptr_t* sp, intptr_t* unextended_sp, intptr_t* fp, bool fp_safe,
    address* sender_pc, intptr_t** sender_sp, intptr_t** sender_unextended_sp, intptr_t** sender_fp) {

  assert(sender_pc != NULL, "invariant");
  assert(sender_sp != NULL, "invariant");

  // fp must be safe
  if (!fp_safe) {
    return false;
  }

  *sender_pc = (address) fp[frame::return_addr_offset];
  // for interpreted frames, the value below is the sender "raw" sp,
  // which can be different from the sender unextended sp (the sp seen
  // by the sender) because of current frame local variables
  *sender_sp = (intptr_t*) &fp[frame::sender_sp_offset];
  if (sender_unextended_sp) *sender_unextended_sp = (intptr_t*) fp[frame::interpreter_frame_sender_sp_offset];
  if (sender_fp) *sender_fp = (intptr_t*) fp[frame::link_offset];

  return true;
}
