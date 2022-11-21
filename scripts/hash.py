def hash(str, init):
  hash = init
  for ch in str:
    hash += (hash << 5) + ord(ch)
  return (hash & 0x7FFFFFFF)

def duplicate(numbers):
  n = len(numbers)
  for i in range(0, n):
    for j in range(i + 1, n):
      if numbers[i] == numbers[j]:
        return -1
  return 0

opes = [
  ['False', 0],
  ['None', 0],
  ['True', 0],
  ['and', 0],
  # ['as', 0],
  ['assert', 0],
  ['break', 0],
  ['class', 0],
  ['continue', 0],
  ['def', 0],
  # ['del', 0],
  ['elif', 0],
  ['else', 0],
  # ['except', 0],
  # ['finally', 0],
  ['for', 0],
  # ['from', 0],
  ['global', 0],
  ['if', 0],
  ['import', 0],
  ['in', 0],
  # ['is', 0],
  ['lambda', 0],
  # ['nonlocal', 0],
  ['not', 0],
  ['or', 0],
  # ['pass', 0],
  # ['raise', 0],
  ['return', 0],
  ['try', 0],
  ['while', 0],
  # ['with', 0],
  # ['yield', 0],
  ['...', 0],
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

f = open("primes.txt","rb")

count = 0

while True:
  line = str(f.readline(), encoding = "utf-8")
  if line == "":
    break
  primes = line.split(" ")
  for p in primes:
    count = count + 1
    if count % 1E+5 == 0:
      print("try prime %s, count %d" % (p[:-1], count))
    for o in opes:
      o[1] = (hash(o[0], int(p)) >> 8) & 0xFF
    opes.sort(key = lambda x : x[1])
    if duplicate([x[1] for x in opes]) == 0:
      print(">>> prime " + p[:-1])
      print(opes)
print("done")
f.close()
