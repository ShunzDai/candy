local function f(n)
  if n < 1 then return 0 end
  return 1 + f(n - 1)
end
print(f($n))
