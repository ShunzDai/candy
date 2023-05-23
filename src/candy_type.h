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
#ifndef CANDY_TYPE_LIST
#error "can only be include by candy_type.list"
#endif /* CANDY_TYPE_LIST */

#ifdef CANDY_TYPE
#undef CANDY_TYPE
#endif /* CANDY_TYPE */

#ifdef CANDY_TYPE_ENUM
#undef CANDY_TYPE_ENUM
#define CANDY_TYPE(_type, ...) TYPE_##_type,
#endif /* CANDY_TYPE_ENUM */

#ifdef CANDY_TYPE_SIZE
#undef CANDY_TYPE_SIZE
#define CANDY_TYPE(_type, _t) sizeof(_t),
#endif /* CANDY_TYPE_SIZE */

#ifdef CANDY_TYPE_STR
#undef CANDY_TYPE_STR
#define CANDY_TYPE(_type, ...) #_type,
#endif /* CANDY_TYPE_STR */

#ifdef CANDY_TYPE_WRAP_FUNCS
#undef CANDY_TYPE_WRAP_FUNCS
#define CANDY_TYPE(_type, _t) \
static inline const _t *candy_wrap_get_##_type(const candy_wrap_t *self) { \
  assert(self->type == TYPE_##_type); \
  return (const _t *)candy_wrap_get_data(self); \
} \
\
static inline void candy_wrap_set_##_type(candy_wrap_t *self, const _t val[], size_t size) { \
  candy_wrap_set_data(self, TYPE_##_type, val, size); \
}
#endif /* CANDY_TYPE_WRAP_FUNCS */

#ifdef CANDY_TYPE_STATE_IMPL
#undef CANDY_TYPE_STATE_IMPL
#define CANDY_TYPE(_type, _t) \
void candy_push_##_type(candy_state_t *self, const _t val[], size_t size) { \
  candy_wrap_t wrap; \
  candy_wrap_set_##_type(&wrap, val, size); \
  candy_vm_push(&self->vm, &wrap); \
} \
\
const _t *candy_pull_##_type(candy_state_t *self, size_t *size) { \
  const candy_wrap_t *wrap = candy_vm_pop(&self->vm); \
  if (size) \
    *size = wrap->size; \
  return candy_wrap_get_##_type(wrap); \
}
#endif /* CANDY_TYPE_STATE_IMPL */

#ifdef CANDY_TYPE_STATE_DECL
#undef CANDY_TYPE_STATE_DECL
#define CANDY_TYPE(_type, _t) \
void candy_push_##_type(candy_state_t *self, const _t *val, size_t size);\
const _t *candy_pull_##_type(candy_state_t *self, size_t *size);
#endif /* CANDY_TYPE_STATE_DECL */
