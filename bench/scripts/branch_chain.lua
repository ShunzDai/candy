local function run()
local i = 0
local sum = 0
while i < $k do
  if i < $first then
    sum = sum + 1
  elseif i < $second then
    sum = sum + 2
  else
    sum = sum + 3
  end
  i = i + 1
end
return sum
end
print(run())
