local function run()
local sum = 0
local i = 0
while i < $k do
  do
    if i < $skip then goto continue_loop end
    if $stop < i then break end
    sum = sum + i
  end
  ::continue_loop::
  i = i + 1
end
return sum
end
print(run())
