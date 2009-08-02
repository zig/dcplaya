
function _dhcp()

local timeout = -1
local maxretries = 5
local retries = maxretries

local valid, yourip, lease_time, subnet_mask, router, domain, dns, dns2
local oldvalid

while valid ~= 5 do
    if timeout < 0 then
	retries = retries - 1
	timeout = maxretries - retries
	if retries < 0 then
	    print "TIMEOUT while getting ip via DHCP"
	    if valid then
		-- still return something
		break
	    end
	    return
	end
	if oldvalid then
	    dhcp_request()
	else
	    dhcp_discover()
	end
    end

    valid, yourip, lease_time, subnet_mask, router, domain, dns, dns2 = dhcp_conf()

    if not oldvalid and valid == 2 then
	oldvalid = valid;

	print("DHCP Offer received")
	print(valid, yourip, lease_time, subnet_mask, router, domain, dns, dns2)

	dhcp_request()

	timeout = maxretries - retries
    end
    
    if peekchar() then
	break
    end

    timeout = timeout - 1.0 / 60
end

if valid == 5 then
    print ("DHCP Ack received")
end
if not valid then
    return
end
print("DHCP Final configuration")
print(valid, yourip, lease_time, subnet_mask, router, domain, dns, dns2)


net_cfg = {
    dhcp = 1,
    ip = yourip,
    mask = subnet_mask,
    gw = router,
    dns = dns,
}

return 1
end

function dhcp()
    dl(plug_dhcp)
    if not dhcp_discover then
	print "failed to load dhcp plugin"
	return
    end
    
    net_connect("0.0.0.0")

    local res = _dhcp()

    -- we're finished with dhcp, unload the driver so that dhcp ip callback is removed and to free
    -- a bit of memory
    driver_unload "dhcp"

    return res
end

return 1
