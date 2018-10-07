-- PINS = { 1, 2, 4 }
PINS = { 4, 5, 6 }

STATION_CFG = {
    ssid="Robotics and Automation Society",
    pwd="IEEE-RAS/LAB"
}

SERVER_PORT = 8000

wifi.setmode(wifi.STATION)

function on_disconnect(connection)
    print("connection is destroyed: " .. tostring(connection))
    for k, v in pairs(connections) do
        if v == connection then
            table.remove(connections, k)
        end
    end
    print("DEBUG: heap = " .. tostring(node.heap()))
    collectgarbage()
end

wifi.eventmon.register(wifi.eventmon.STA_GOT_IP, function (T)
    print("DEBUG: heap = " .. tostring(node.heap()))
    print("we have got IP: " .. T.IP)
    server = net.createServer()
    server:listen(SERVER_PORT, function (connection)
        print("new connection..." .. tostring(connection))
        print("DEBUG: heap = " .. tostring(node.heap()))
        connection:on("disconnection", on_disconnect)
        table.insert(connections, connection)
    end)
end)

wifi.sta.config(STATION_CFG)

connections = {}

function do_send(dummy)
    s = ""
    for _, pin in pairs(PINS) do
        s = s .. tostring(gpio.read(pin))
    end
    
    s = s .. "\n"
    
    for _, c in pairs(connections) do
        c:send(s)
    end
    collectgarbage()
    print("DEBUG: heap = " .. tostring(node.heap()))
end

for _, pin in pairs(PINS) do
    gpio.mode(pin, gpio.INT)
    gpio.trig(pin, "both", do_send)
end

