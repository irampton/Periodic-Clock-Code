const { SerialPort } = require( 'serialport' );
const { ReadlineParser } = require( '@serialport/parser-readline' );
let port, parser
let connected = false;
let scanInterval;

module.exports = {
    //TODO add a listener for key presses
    scan,
    connect,
    stopScan,
    send,
    invoke,
    on,
    onClose: setOnClose,
    disconnect
}

let onNextJson;
let onMsg = {};
let onClose;

function onMessage( msg ) {
    console.log(msg);
    if ( onNextJson && ( msg[0] === "{" || msg[0] === "[" ) ) {
        onNextJson( JSON.parse( msg ) );
        onNextJson = "";
    } else if ( Object.keys( onMsg ).includes( msg.match( /^\w*/ )[0] ) ) {
        onMsg[msg.match( /^\w*/ )[0]]( msg );
    } else {
        console.log( "Unknown command from serial:\t", msg );
    }
}

function on( command, callback ) {
    [command] = callback;
}

function setOnClose( callback ) {
    onClose = callback
}


function scan( callback ) {
    scanInterval = setInterval( async () => {
        callback( await SerialPort.list() );
    }, 2000 );
}

function stopScan() {
    clearInterval( scanInterval );
    scanInterval = null;
}

function connect( path ) {
    return new Promise( ( resolve, reject ) => {
        stopScan();
        // Must match Serial.begin(9600) in the Pico firmware.
        const newPort = new SerialPort( { path: path, baudRate: 9600, autoOpen: false, } );
        port = newPort;
        newPort.on( 'open', function () {
            connected = true;
            resolve();
        } );
        newPort.open( function ( err ) {
            if ( err ) {
                if ( port === newPort ) {
                    port = null;
                }
                reject( err );
            }
        } )
        const newParser = newPort.pipe( new ReadlineParser( { delimiter: '\r\n' } ) );
        parser = newParser;
        newParser.on( 'data', onMessage );
        newPort.on( 'close', () => {
            // A previous port can finish closing after the next one has opened.
            // Only clear the shared state when this is still the active port.
            if ( port === newPort ) {
                connected = false;
                port = null;
                parser = null;
            }
            if ( typeof onClose === "function" ) {
                onClose();
            }
        } )
    } )
}

function send( msg ) {
    //TODO check to see if initialized

    try {
        //console.log( "sending: ", msg );
        if ( !connected || !port || !port.isOpen ) {
            throw new Error( 'Serial port is not connected' );
        }
        port.write( msg );
        port.write( '\r\n' );
    } catch ( e ) {
        console.error( "error sending serial port data", e );
        return false;
    }
    return true;
}

function invoke( msg ) {
    return new Promise( ( resolve ) => {
        send( msg );
        onNextJson = list => {
            resolve( list );
        };
    } );
}

function disconnect(){
    const currentPort = port;
    if ( !currentPort || !currentPort.isOpen ) {
        return Promise.resolve();
    }

    return new Promise( ( resolve, reject ) => {
        currentPort.close( err => {
            if ( err ) {
                reject( err );
            } else {
                resolve();
            }
        } );
    } );
}
