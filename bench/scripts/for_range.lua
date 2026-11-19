local function run()
  local arr = {$elems}
  local sum = 0
  for i = 1, #arr do sum = sum + arr[i] end
  return sum
end
print(run())
