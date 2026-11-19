local function run()
local i = 0
local sum = 0
while i < $k do
  sum = sum + i
  i = i + 1
end
return sum
end
print(run())
