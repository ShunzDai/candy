"""Correctness-only cases not used for instruction-layout tuning."""
CASES = [
    ('argument_order', '''def id(x)
  return x
end
def add(a, b)
  return a + b
end
print(add(id(7) - id(2), add(id(3), id(4))))
print(add(1 + id(2), id(4) - 1))
''', '12\n6\n'),
    ('local_survives_call', '''def id(x)
  return x
end
def f(a, b)
  return id(a + b) + a - b
end
print(f(13, 4))
''', '26\n'),
    ('function_assignment_is_local', '''def original(x)
  return 10 + x
end
def replacement(x)
  return 20 + x
end
f = original
def change()
  f = replacement
  return 1
end
print(f(change()))
print(f(1))
''', '11\n11\n'),
]
