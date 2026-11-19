function even(n)
  if n < 1 then return 1 end
  return odd(n - 1)
end
function odd(n)
  if n < 1 then return 0 end
  return even(n - 1)
end
$body
print(even($depth))
