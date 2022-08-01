def hash00(_l, _r):
  return _l + _r

def hash01(_l, _r):
  return _l | _r

def hash02(_l, _r):
  return _l ^ _r

def hash03(_l, _r):
  return _l & _r

def hash04(_l, _r):
  return ((_l * _r) % 255) | 0x80

def hash05(_l, _r):
  return _l / _r

def hash06(_l, _r):
  return _l % _r

def hash07(_l, _r):
  return ~_l + _r

def hash08(_l, _r):
  return ~_l | _r

def hash09(_l, _r):
  return ~_l ^ _r

def hash10(_l, _r):
  return ~_l & _r

def hash11(_l, _r):
  return ~_l * _r

def hash12(_l, _r):
  return ~_l / _r

def hash13(_l, _r):
  return ~_l % _r

def hash14(_l, _r):
  return _l + ~_r

def hash15(_l, _r):
  return _l | ~_r

def hash16(_l, _r):
  return _l ^ ~_r

def hash17(_l, _r):
  return _l & ~_r

def hash18(_l, _r):
  return _l * ~_r

def hash19(_l, _r):
  return _l / ~_r

def hash20(_l, _r):
  return _l % ~_r

def hash21(_l, _r):
  return ~_l + ~_r

def hash22(_l, _r):
  return ~_l | ~_r

def hash23(_l, _r):
  return ~_l ^ ~_r

def hash24(_l, _r):
  return ~_l & ~_r

def hash25(_l, _r):
  return ~_l * ~_r

def hash26(_l, _r):
  return ~_l / ~_r

def hash27(_l, _r):
  return ~_l % ~_r

def duplicate(numbers):
  n = len(numbers)
  for i in range(0, n):
    for j in range(i + 1, n):
      if numbers[i] == numbers[j]:
        return True
  return False

funcs = [hash00, hash01, hash02, hash03, hash04, hash05, hash06, hash07, hash08, hash09, hash10, hash11, hash12, hash13, hash14, hash15, hash16, hash17, hash18, hash19, hash20, hash21, hash22, hash23, hash24, hash25, hash26, hash27]

ope = ['**', '//', '!=', '%=', '*=', '+=', '-=', '/=', '<<', '<=', '==', '>=', '>>']

for f in funcs:
  hash = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  i = 0
  for o in ope:
    hash[i] = f(ord(o[0]), ord(o[1]))
    i += 1
  if duplicate(hash):
    print('hash collision')
  else:
    print('hash success function: ' + f.__name__)
    print(hash)
