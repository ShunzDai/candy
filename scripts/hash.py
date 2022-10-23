def hash_DJB(str):
  hash = 5381
  for ch in str:
    hash += (hash << 5) + ord(ch)
  return (hash & 0x7FFFFFFF)

def hash_BKDR(str):
  hash = 0
  for ch in str:
    hash = hash * 131 + ord(ch)
  return (hash & 0x7FFFFFFF)

def hash_SDBM(str):
  hash = 0
  for ch in str:
    hash = (hash << 6) + (hash << 16) - hash + ord(ch)
  return (hash & 0x7FFFFFFF)

def duplicate(numbers):
  n = len(numbers)
  for i in range(0, n):
    for j in range(i + 1, n):
      if numbers[i] == numbers[j]:
        return numbers[i]
  return 0

ope = [
  ['False', 0],
  ['None', 0],
  ['True', 0],
  ['and', 0],
  ['as', 0],
  ['assert', 0],
  ['break', 0],
  ['class', 0],
  ['continue', 0],
  ['def', 0],
  ['del', 0],
  ['elif', 0],
  ['else', 0],
  ['except', 0],
  ['finally', 0],
  ['for', 0],
  ['from', 0],
  ['global', 0],
  ['if', 0],
  ['import', 0],
  ['in', 0],
  ['is', 0],
  ['lambda', 0],
  ['nonlocal', 0],
  ['not', 0],
  ['or', 0],
  ['pass', 0],
  ['raise', 0],
  ['return', 0],
  ['try', 0],
  ['while', 0],
  ['with', 0],
  ['yield', 0],
  ['**', 0],
  ['//', 0],
  ['!=', 0],
  ['%=', 0],
  ['*=', 0],
  ['+=', 0],
  ['-=', 0],
  ['/=', 0],
  ['<<', 0],
  ['<=', 0],
  ['==', 0],
  ['>=', 0],
  ['>>', 0],
]

for o in ope:
  o[1] = hash_DJB(o[0]) & 0xFF | 0x80
ope.sort(key = lambda x : x[1])
print(duplicate([x[1] for x in ope]))
print(ope)

