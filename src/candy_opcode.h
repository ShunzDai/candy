/**
  * Copyright 2022-2023 ShunzDai
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#ifndef CANDY_OPCODE_LIST
#error "can only be include by candy_opcode.list"
#endif /* CANDY_OPCODE_LIST */

#ifdef CANDY_OP
#undef CANDY_OP
#endif /* CANDY_OP */

#if defined(CANDY_OP_ENUM)
#undef CANDY_OP_ENUM
#define CANDY_OP(_opcode, ...) OP_##_opcode,
#endif /* CANDY_OP_ENUM */

#if defined(CANDY_OP_CASE)
#undef CANDY_OP_CASE
#define CANDY_OP(_opcode, ...) case OP_##_opcode: { \
  __VA_ARGS__ \
}
#endif /* CANDY_OP_CASE */
