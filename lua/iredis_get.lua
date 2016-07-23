local redis = require "resty.iredis"

local red = redis:new({["ip"]="127.0.0.1", ["port"]=6379})

function get_from_redis(key)
    local res, err = red:get(key)
    if res then
        return res
    else
        return 'unknown'
    end
end


function set_to_cache(key, value, exptime)
    if not exptime then
        exptime = 0
    end
    local cache_ngx = ngx.shared.cache_ngx
    local succ, err, farcible = cache_ngx:set(key, value, exptime)
    return succ
end

function get_from_cache(key)
    local cache_ngx = ngx.shared.cache_ngx
    local value = cache_ngx:get(key)
    if not value then
        value = get_from_redis(key)
        set_to_cache(key, value)
    end
    return value
end


local res = get_from_redis('cat')
ngx.say(res)
