#include "src/candy_reader.h"
#include <string.h>

int string_reader(char *buff, const int max_len, void *ud) {
  struct str_info *info = (struct str_info *)ud;
  int residual = info->size - info->offset;
  int len = (max_len > residual) ? residual : max_len;
  memcpy(buff, info->exp + info->offset, len);
  info->offset += len;
  if (info->offset == info->size && len < max_len)
    buff[len] = (char)EOF;
  return len;
}

int file_reader(char *buff, const int max_len, void *ud) {
  struct file_info *info = (struct file_info *)ud;
  int residual = info->size - ftell(info->f);
  int len = (max_len > residual) ? residual : max_len;
  fread(buff, sizeof(char), len, info->f);
  if ((int)ftell(info->f) == info->size && len < max_len)
    buff[len] = (char)EOF;
  return len;
}
