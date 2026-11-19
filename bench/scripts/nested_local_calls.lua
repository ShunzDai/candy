local function run()
  local function id(n)
    return n
  end
$body
  return id(1)
end
print(run())
