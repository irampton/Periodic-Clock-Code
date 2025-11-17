const Serial = require('./serial');

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function getTimezoneInfo() {
    const now = new Date();
    const january = new Date(now.getFullYear(), 0, 1);
    const july = new Date(now.getFullYear(), 6, 1);

    // JavaScript reports offsets in minutes behind UTC; convert to minutes east of UTC
    const standardOffset = Math.max(january.getTimezoneOffset(), july.getTimezoneOffset());
    const usesDst = january.getTimezoneOffset() !== july.getTimezoneOffset();

    return {
        offsetMinutes: -standardOffset,
        usesDst,
    };
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

                const epochSeconds = Math.floor(Date.now() / 1000);
                const { offsetMinutes, usesDst } = getTimezoneInfo();
                const hourOffset = Math.trunc(offsetMinutes / 60);

                console.log(`Setting time with epoch=${epochSeconds}, hour offset=${hourOffset}, dst=${usesDst}`);
                Serial.send(`set time ${epochSeconds} ${hourOffset} ${usesDst ? 1 : 0}`);

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
