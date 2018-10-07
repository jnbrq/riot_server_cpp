RIOT_DATA = [[
riotp300
name: window_sensor
END
s ACTION_REFRESH
]]

RIOT_CFG = {
    address="192.168.1.117",
    port=8000
}

STATION_CFG = {
    ssid="Robotics and Automation Society",
    pwd="IEEE-RAS/LAB"
}

PINS = { }
PINS["1"] = 1
PINS["2"] = 2
PINS["3"] = 5

conn = net.createConnection()

function do_send()
    for k, v in pairs(PINS) do
        conn:send(
            "t EVT_WINDOW"..tostring(k).."\n"..
            tostring(gpio.read(v)) .. "\n")
    end
end

function on_riot_connected()
    print("connected...")
    conn:send(RIOT_DATA)
    for _, pin in pairs(PINS) do
        gpio.mode(pin, gpio.INT)
        gpio.trig(pin, "both", do_send)
    end
end

function on_receive(s, payload)
    print(payload)
    if payload:find("^ee") ~= nil then
        do_send()
    end
end

function on_wifi_connected(T)
    print("WiFİ connected with IP: " .. T.IP)
    conn:on("connection", on_riot_connected)
    conn:on("receive", on_receive)
    conn:connect(RIOT_CFG.port, RIOT_CFG.address)
end

wifi.eventmon.register(wifi.eventmon.STA_GOT_IP, on_wifi_connected)

wifi.setmode(wifi.STATION)
wifi.sta.config(STATION_CFG)

