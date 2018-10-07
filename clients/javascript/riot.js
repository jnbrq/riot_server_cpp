/**
 *
 * Canberk Sönmez
 * A simple RIOT implementation
 * Supports only triggering and receiving functionality with single line messages
 * no caching, pause/resume, etc.
 */

function RIOTclient(config) {
    function log(o) {
        console.log("[riot.js] " + o);
    }
    
    function error(o) {
        throw "[riot.js] " + o;
    }
    
    if (!("name" in config))
        error("config.name must exist!");
    if (!("groups" in config))
        config["groups"] = "monitor";
    
    var cfg = config;
    var s10ns_raw = [];
    var s10ns_idx = 0;
    var s10ns = {};
    var started = false;
    var active = false;
    var trigger_queue = [];
    var trigger_state = 0;  // 0: means that the header is sent
                            // 1: means that the data is sent
    
    var event = { "hdr": null };  // current event
    var scheduled = [];
    var skip_count = 0;
    var ws = {};

    function send_lines(lines, cb) {
        function send_next() {
            if (lines.length == 0) {
                ws.o.onmessage = null;
                cb();
            }
            else {
                var next = lines.shift();
                log("[send_lines] sending: " + next);
                ws.o.send(next);
            }
        }
        ws.o.onmessage = function (e) {
            if (typeof e.data === "string") {
                log("[send_lines] received: " + e.data);
                var line = e.data.trim();
                if (line.startsWith("ok")) {
                    send_next();
                }
                else {
                    error("[send_lines] non-ok received!")
                }
            }
        };
        send_next();
    }
    
    function send_s10ns(cb) {
        function send_next() {
            if (s10ns_idx == s10ns_raw.length) {
                // done
                ws.o.onmessage = null;
                cb();
            }
            else {
                ws.o.send("s " + s10ns_raw[s10ns_idx][0]);
                log("[send_s10ns] sending: " + s10ns_raw[s10ns_idx][0]);
            }
        }
        ws.o.onmessage = function (e) {
            if (typeof e.data == "string") {
                log("[send_s10ns] received: " + e.data);
                var line = e.data.trim();
                if (line.startsWith("ok")) {
                    var v = e.data.trim().split(/\s+/);
                    var id = parseInt(v[1]);
                    s10ns[id] = s10ns_raw[s10ns_idx][1];
                    s10ns_idx++;
                    send_next();
                }
                else {
                    error("[send_s10ns] non-ok received!")
                }
            }
        }
        send_next();
    }
    
    function send_trigger_header() {
        trigger_state = 0;  // first header
        if (trigger_queue[0][1] === null) {
            // null data, send te
            ws.o.send("te " + trigger_queue[0][0]);
        }
        else {
            // trigger line was sent
            ws.o.send("t " + trigger_queue[0][0]);
        }
    }
    
    function send_trigger_data() {
        trigger_state = 1;  // next data
        ws.o.send(trigger_queue[0][1]);
    }
    
    function parse_event_line(line) {
        var v = line.split(/\s+/);
        var r = {"evt": v[1], "sender": v[2], "ids": []};
        for (var i = 3; i < v.length; i++) {
            r.ids.push(v[i]);
        }
        return r;
    }
    
    function invoke_triggers(hdr, data) {
        for (var i = 0; i < hdr.ids.length; ++i) {
            s10ns[hdr.ids[i]](hdr.evt, hdr.sender, data);
        }
    }
    
    function on_message(e) {
        if (skip_count > 0) {
            skip_count--;
        }
        if (typeof e.data === "string") {
            var line = e.data.trim();
            log("[on_message] received: " + line);
            if (event.hdr !== null) {
                // previous message was an el
                // current message is the data :)
                invoke_triggers(event.hdr, e.data);
                event.hdr = null;
            }
            else if (line.startsWith("el")) {
                // this line is a trigger line
                event.hdr = parse_event_line(line);
            }
            else if (line.startsWith("ee")) {
                // this line is a trigger empty
                event.hdr = null;
                var hdr = parse_event_line(line);
                invoke_triggers(hdr, null);
            }
            else if (line.startsWith("ok")) {
                // we are triggering something
                if (trigger_state == 1) {
                    // data has been sent
                    trigger_queue.shift();
                    if (trigger_queue.length > 0) {
                        send_trigger_header();
                    }
                }
                else if (trigger_state == 0) {
                    // header has been sent
                    if (trigger_queue[0][1] === null) {
                        // te was sent
                        trigger_queue.shift();
                        if (trigger_queue.length > 0) {
                            send_trigger_header();
                        }
                    }
                    else {
                        send_trigger_data();
                    }
                }
            }
            else if (line.startsWith("err") || line.startsWith("warn")) {
                log("Connection returned: " + line.data);
                // TODO is it really the correct way to handle this?
                trigger_queue.shift();
                if (trigger_queue.length > 0) {
                    send_trigger_header();
                }
            }
        }
    }
    
    this.trigger = function (evt, data) {
        if (!active) {
            error("Connection is not active, trigger cannot be called");
        }
        
        if (data !== null) {
            if (data.includes("\n")) {
                error("Data containing new lines is not supported!");
            }
        }
        
        trigger_queue.push([evt, data]);
        
        if (trigger_queue.length == 1) {
            send_trigger_header();
        }
    };
    
    this.hasStarted = function () {
        return started;
    }
    
    this.isActive = function () {
        return active;
    }
    
    this.subscribe = function (evt, f) {
        if (started) {
            error("Connection is started, further subscriptions are not allowed!")
        }
        if (!(typeof f === "function")) {
            error("A function is strictly required!");
        }
        s10ns_raw.push([evt, f]);
    };
    
    this.schedule = function (f) {
        scheduled.push(f);
    }
    
    this.start = function () {
        if (started) {
            error("Connection can be started only once!")
        }
        
        ws.o = new WebSocket(cfg.address);
        ws.o.onopen = function () {
            started = true;
            log("sending header");
            send_lines([
                "riotp300",
                "name: " + cfg.name,
                "groups: " + cfg.groups,
                "requires: textonly",
                "END"
            ], function () {
                log("sending header done");
                log("sending subscriptions");
                send_lines(["pause"], function() {
                    send_s10ns(function () {
                        send_lines(["resume"], function () {
                            log("sending subscriptions done");
                            ws.o.onmessage = on_message;
                            active = true;
                            // now start the scheduled functions
                            for (var i = 0; i < scheduled.length; ++i) {
                                var f = scheduled[i];
                                if (typeof f === "function") {
                                    f();
                                }
                            }
                        });
                    });
                });
            });
        };
    };
}
