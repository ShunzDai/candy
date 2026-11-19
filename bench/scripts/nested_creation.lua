local function run()
  local i = 0
  local sum = 0
  while i < $k do
    local function helper(n)
      return n
    end
    sum = sum + helper(i)
    i = i + 1
  end
  return sum
end
print(run())
