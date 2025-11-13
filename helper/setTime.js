const Serial = require('./serial');

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function sendToAllPorts() {
    Serial.scan(async (ports) => {
        if (!Array.isArray(ports) || ports.length === 0) {
            console.error('No serial ports found.');
            return;
        }

        // We got the list; no need to keep scanning.
        Serial.stopScan();

        for (const port of ports) {
            if (!port || !port.path) {
                console.warn('Skipping port with no path:', port);
                continue;
            }

            console.log(`\n=== Trying port: ${port.path} ===`);

            try {
                await Serial.connect(port.path);
                console.log(`Connected to ${port.path}`);

                const now = new Date();
                const hour = now.getHours();
                const min  = now.getMinutes();
                const sec  = now.getSeconds();

                console.log(hour);
                Serial.send(`set hour ${hour}`);

                console.log(min);
                Serial.send(`set min ${min}`);

                console.log(sec);
                Serial.send(`set sec ${sec}`);

                Serial.send('hello world');

                // Give it some time to transmit / process if needed
                await sleep(1000);

                Serial.disconnect();
                console.log(`Disconnected from ${port.path}`);
            } catch (error) {
                console.error(`Failed on serial port ${port.path}:`, error);
                // Move on to the next port, no rethrow
                try {
                    // Just in case a partial connection happened
                    Serial.disconnect();
                } catch (_) {
                    // ignore disconnect errors
                }
            }
        }

        console.log('\nDone processing all serial ports.');
    });
}

sendToAllPorts();
