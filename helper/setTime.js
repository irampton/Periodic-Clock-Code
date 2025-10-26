const Serial = require('./serial');

function sendHelloWorld() {
    Serial.scan(async ports => {
        if (!Array.isArray(ports) || ports.length === 0) {
            return;
        }

        Serial.stopScan();

        const [x, firstPort] = ports;

        if (!firstPort || !firstPort.path) {
            console.error('First serial port has no path; aborting connection attempt.');
            return;
        }

        try {
            await Serial.connect(firstPort.path);
            console.log(`Connected to ${firstPort.path}`);
            console.log(new Date().getHours());
            Serial.send(`set hour ${new Date().getHours()}`);
            console.log(new Date().getMinutes());
            Serial.send(`set min ${new Date().getMinutes()}`);
            console.log(new Date().getSeconds());
            Serial.send(`set sec ${new Date().getSeconds()}`);
            Serial.send('hello world');
            setTimeout(()=>{
                Serial.disconnect();
            }, 10000);
        } catch (error) {
            console.error(`Failed to connect to serial port ${firstPort.path}:`, error);
        }
    });
}

sendHelloWorld();
