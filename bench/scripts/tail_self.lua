local function run()
  local function count(n, total)
    if n < 1 then
      return total
    end
    return count(n - 1, total + n)
  end
  return count($k, 0)
end
print(run())
