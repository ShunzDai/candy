function fib(n)
  if n < 2 then return n end
  return fib2(n - 2) + fib2(n - 1)
end
fib2 = fib
print(fib($n))
