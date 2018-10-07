RIOT_DATA = [[
riotp300
name: temp_sensor
groups: ras
END
]]

RIOT_CFG = {
    address="192.168.1.117",
    port=8000
}

STATION_CFG = {
    ssid="Robotics and Automation Society",
    pwd="IEEE-RAS/LAB"
}

conn = net.createConnection()

function normalize(t)
    return math.floor(3300*tonumber(t)/1024)/10
end

function on_riot_connected()
    print("connected...")
    conn:send(RIOT_DATA)
    t0 = tmr.create()
    t0:register(1000, tmr.ALARM_AUTO, function ()
        conn:send("t EVT_TEMP\n" .. tostring(normalize(adc.read(0))) .. "\n")
    end)
    t0:start()
end

function on_receive(s, payload)
    print(payload)
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

