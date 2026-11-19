local function run()
local function id(n)
  return n
end
local i = 0
local sum = 0
while i < $k do
  sum = sum + id(i)
  i = i + 1
end
return sum
end
print(run())
