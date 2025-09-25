local func = {}

function func.imap(f, iter)
    local index = 1
    if type(iter) == 'function' then
        return function()
            local item = iter()
            if item then
                return f(item)
            end
        end
    elseif type(iter) == 'table' then
        return function()
            local cur = index
            if cur <= #iter then
                index = index + 1
                return f(iter[cur])
            end
        end
    else
        error('unsupport type ' .. type(iter))
    end
end

function func.chains(iter)
    local cur_iter = iter()
    return function()
:: again ::  local res = cur_iter()
        if res then
            return res
        end
        cur_iter = iter()
        if cur_iter then
            goto again
        end
    end
end

function func.list(...)
  local arr = {}
  for v in ... do
    arr[#arr + 1] = v
  end
  return arr
end

return func
