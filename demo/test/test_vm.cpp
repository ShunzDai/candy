/**
  * Copyright 2022 ShunzDai
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
#include "gtest/gtest.h"
#include "src/common/candy_lib.h"
#include "src/struct/candy_object.h"
#include "src/method/candy_standard.h"
#include "src/core/candy_vm.h"

TEST(vm, annotate){
  candy_object_t root = candy_object_create();
  candy_vm_runcode(root, (char *)
    "# annotate start\r\n"
  );
  candy_vm_runcode(root, (char *)
    "'''\r\n"
    "print(\"annotate\")\r\n"
    "'''\r\n"
  );
  candy_vm_runcode(root, (char *)
    "\"\"\"\r\n"
    "print(\"annotate\")\r\n"
    "\"\"\"\r\n"
  );
  candy_vm_runcode(root, (char *)
    "# annotate start\r\n"
    "'''\r\n"
    "print(\"annotate\")\r\n"
    "'''\r\n"
    "\"\"\"\r\n"
    "print(\"annotate\")\r\n"
    "\"\"\"\r\n"
  );
  candy_object_delete(root);
}

TEST(vm, string){
  candy_object_t root = candy_object_create();
  candy_vm_runcode(root, (char *)
    "print(\"hello world\\n\")\r\n"
  );
  candy_vm_runcode(root, (char *)
    "print(\'hello world\\n\')\r\n"
  );
  candy_vm_runcode(root, (char *)
    "print(\'AB\\\"CD\')\r\n"/* print('\"') */
    "print(\'AB\\\'CD\')\r\n"/* print('\'') */
    "print(\"AB\\\"CD\")\r\n"/* print("\"") */
    "print(\"AB\\\'CD\")\r\n"/* print("\'") */
  );
  candy_object_delete(root);
}

TEST(vm, hex){
  candy_object_t root = candy_object_create();
  candy_vm_runcode(root, (char *)
    "print(\"\\x30\\x31\\x32\\x33\")\r\n"
  );
  candy_vm_runcode(root, (char *)
    "print(\"0\\x3123\")\r\n"
  );
  candy_object_delete(root);
}

TEST(vm, dec){
  candy_object_t root = candy_object_create();
  candy_vm_runcode(root, (char *)
    "print(\"123\")\r\n"
  );
  candy_vm_runcode(root, (char *)
    "print(\"123.456\")\r\n"
  );
  candy_object_delete(root);
}

TEST(vm, oct){
  candy_object_t root = candy_object_create();
  candy_vm_runcode(root, (char *)
    "print(\"\\061\\062\\063\\141\\142\\143\")\r\n"
  );
  candy_vm_runcode(root, (char *)
    "print(\"01\\0623\")\r\n"
  );
  candy_object_delete(root);
}
