function id(n)
  return n
end
local function run(callee)
local i = 0
local sum = 0
while i < $k do
  sum = sum + callee(i)
  i = i + 1
end
return sum
end
print(run(id))
