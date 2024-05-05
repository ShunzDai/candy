keywords = [
  ["true", 0],
  ["false", 0],
  ["none", 0],
  ["def", 0],
  ["return", 0],
  ["end", 0],
  ["import", 0],
  ["if", 0],
  ["elif", 0],
  ["else", 0],
  ["while", 0],
  ["for", 0],
  ["break", 0],
  ["continue", 0],
]

def djb_hash(str):
  hash = 5381
  for ch in str:
    hash += (hash << 5) + ord(ch)
  return hash & 0x7FFFFFFF

for key in keywords:
  key[1] = djb_hash(key[0])

# keywords.sort(key = lambda x : x[1])

for key in keywords:
  print("CANDY_KW(%*s, 0x%08XU)" % (8, key[0], key[1]))
