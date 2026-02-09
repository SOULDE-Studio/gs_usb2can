// CRC32 implementation
function generate_crc32(data) {
    const polynomial = 0xEDB88320;
    let crc = 0xFFFFFFFF;

    for (let i = 0; i < data.length; i++) {
        crc ^= data[i];

        for (let j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >>> 1) ^ polynomial;
            } else {
                crc >>>= 1;
            }
        }
    }

    return (crc ^ 0xFFFFFFFF) >>> 0;
}

// DOM elements
const serialPortSelect = document.getElementById('serial-port');
const refreshPortsBtn = document.getElementById('refresh-ports');
const firmwareFileInput = document.getElementById('firmware-file');
const useCrcCheckbox = document.getElementById('use-crc');
const connectBtn = document.getElementById('connect-btn');
const infoBtn = document.getElementById('info-btn');
const flashBtn = document.getElementById('flash-btn');
const disconnectBtn = document.getElementById('disconnect-btn');
const bootloaderInfoDiv = document.getElementById('bootloader-info');
const progressFill = document.getElementById('progress-fill');
const progressText = document.getElementById('progress-text');
const statusText = document.getElementById('status-text');
const consoleOutput = document.getElementById('console-output');

// Global variables
let firmwareData = null;
let firmwareSize = 0;
let crc32Value = null;

// Log function
const logHistory = [];
const MAX_LOG_ENTRIES = 50;

function log(message) {
    // Add to history
    logHistory.push(message);

    // Keep only the latest 50 entries
    if (logHistory.length > MAX_LOG_ENTRIES) {
        logHistory.shift();
    }

    // Update console output
    consoleOutput.innerHTML = logHistory.join('\n');
    consoleOutput.scrollTop = consoleOutput.scrollHeight;
    console.log(message);
}

// Update progress
function updateProgress(percentage, status) {
    progressFill.style.width = percentage + '%';
    progressText.textContent = percentage + '%';
    statusText.textContent = status;
}

// Real serial port detection using Web Serial API
let port = null;
let writer = null;
let reader = null;
let isConnected = false;

// Receive queue to store all incoming data
const receiveQueue = {
    data: new Uint8Array(0),
    readIndex: 0,

    // Add data to queue
    add(data) {
        const newData = new Uint8Array(this.data.length + data.length);
        newData.set(this.data);
        newData.set(data, this.data.length);
        this.data = newData;
        // console.log(`Added ${data.length} bytes to queue, total: ${this.data.length}`);
    },

    // Get data from queue
    get(length) {
        if (this.data.length - this.readIndex < length) {
            return null;
        }

        const result = this.data.slice(this.readIndex, this.readIndex + length);
        this.readIndex += length;

        // If we've read all data, reset
        if (this.readIndex >= this.data.length) {
            this.data = new Uint8Array(0);
            this.readIndex = 0;
        }

        // console.log(`Retrieved ${length} bytes from queue, remaining: ${this.data.length - this.readIndex}`);
        return result;
    },

    // Get queue size
    size() {
        return this.data.length - this.readIndex;
    },

    // Clear queue
    clear() {
        this.data = new Uint8Array(0);
        this.readIndex = 0;
        // console.log('Queue cleared');
    }
};

async function refreshSerialPorts() {
    // Debug: Check if function is called
    console.log('refreshSerialPorts function called');

    // Check if Web Serial API is supported
    if (!navigator.serial) {
        log('Web Serial API is not supported in this browser');
        log('Please use Chrome, Edge, or Opera');
        return;
    }

    log('Refreshing serial ports...');

    // Clear existing options
    serialPortSelect.innerHTML = '<option value="">-- Select Port --</option>';

    try {
        // Get all serial ports
        const ports = await navigator.serial.getPorts();

        if (ports.length === 0) {
            log('No serial ports found. Requesting port access...');
            // Try to request a new port
            try {
                const newPort = await navigator.serial.requestPort();
                if (newPort) {
                    // Create a new array with the new port
                    const newPorts = [newPort];
                    // Add the new port to dropdown
                    newPorts.forEach(serialPort => {
                        const option = document.createElement('option');
                        const info = serialPort.getInfo();
                        option.value = newPorts.indexOf(serialPort).toString();
                        option.textContent = `Serial Port ${info.usbVendorId ? 'USB VID: ' + info.usbVendorId.toString(16).toUpperCase() : 'Unknown'} ${info.usbProductId ? 'PID: ' + info.usbProductId.toString(16).toUpperCase() : ''}`;
                        serialPortSelect.appendChild(option);
                    });
                    log('New serial port access granted');
                    log(`Found 1 serial port(s)`);
                } else {
                    log('No port selected. Please try again.');
                }
            } catch (error) {
                log('No port access granted. Please try again.');
                // Keep the default option in dropdown
                return;
            }
        } else {
            // Add each port to dropdown
            ports.forEach(serialPort => {
                const option = document.createElement('option');
                const info = serialPort.getInfo();
                option.value = ports.indexOf(serialPort).toString();
                option.textContent = `Serial Port ${info.usbVendorId ? 'USB VID: ' + info.usbVendorId.toString(16).toUpperCase() : 'Unknown'} ${info.usbProductId ? 'PID: ' + info.usbProductId.toString(16).toUpperCase() : ''}`;
                serialPortSelect.appendChild(option);
            });

            log(`Found ${ports.length} serial port(s)`);
        }
    } catch (error) {
        log('Error refreshing serial ports: ' + error.message);
        // Keep the default option in dropdown
    }
}

// Real connection using Web Serial API
async function connectToDevice() {
    const selectedIndex = serialPortSelect.value;
    if (selectedIndex === "") {
        log('Please select a serial port first');
        return;
    }

    // Get all ports and select the one user chose
    const ports = await navigator.serial.getPorts();
    if (ports.length === 0) {
        log('No serial ports available. Please click "Refresh" to request port access.');
        return;
    }

    const index = parseInt(selectedIndex);
    if (index < 0 || index >= ports.length) {
        log('Invalid serial port selected. Please click "Refresh" and try again.');
        return;
    }

    port = ports[index];

    // Open the serial port with baud rate 10000000
    try {
        await port.open({ baudRate: 10000000 });

        // Create a writer
        writer = port.writable.getWriter();
        // Create a reader
        reader = port.readable.getReader();

        isConnected = true;
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;
        infoBtn.disabled = false;
        flashBtn.disabled = false;

        log(`Connected to serial port`);
        updateProgress(0, 'Connected');

        // Start continuous reading loop
        startReadingLoop();
    } catch (error) {
        log('Error connecting to device: ' + error.message);
        log('Please make sure the device is connected and try again.');
    }
}

// Continuous reading loop to add data to queue
async function startReadingLoop() {
    if (!reader) return;

    try {
        while (isConnected) {
            const { value, done } = await reader.read();
            if (done) {
                log('Reader closed');
                break;
            }

            if (value) {
                receiveQueue.add(value);
                // console.log(`Received ${value.length} bytes, queue size: ${receiveQueue.size()}`);
            }
        }
    } catch (error) {
        console.error('Error in reading loop:', error);
        if (isConnected) {
            // If connection is still open, restart reading loop
            setTimeout(startReadingLoop, 1000);
        }
    }
}

// Real disconnection using Web Serial API
function disconnectFromDevice() {
    if (!port) return;

    // Release the writer and reader
    if (writer) {
        writer.releaseLock();
        writer = null;
    }

    if (reader) {
        reader.releaseLock();
        reader = null;
    }

    // Close the port
    port.close()
        .then(() => {
            isConnected = false;
            connectBtn.disabled = false;
            disconnectBtn.disabled = true;
            infoBtn.disabled = true;
            flashBtn.disabled = true;

            log('Disconnected from serial port');
            updateProgress(0, 'Disconnected');
        })
        .catch(error => {
            log('Error disconnecting: ' + error.message);
        });
}

// Real bootloader info retrieval using Web Serial API
function getBootloaderInfo() {
    if (!isConnected || !writer || !reader) {
        log('Please connect to device first');
        return;
    }

    log('Getting bootloader info...');
    bootloaderInfoDiv.innerHTML = '<p>Getting info...</p>';

    // Clear info first
    let bootloaderVersion = 'Unknown';
    let productSeries = 'Unknown';
    let productId = 'Unknown';

    // Function to send command and receive response
    async function sendCommand(command) {
        // Create frame
        const frame = new Uint8Array(8);
        frame[0] = 0x04; // Command type
        frame[1] = 0x00;
        frame[2] = 0x00;
        frame[3] = 0x01;
        frame[4] = command; // Command ID
        frame[5] = 0x00;
        frame[6] = 0x00;
        frame[7] = 0x00;

        // Send frame
        await writer.write(frame);
        // log(`Sent command: ${command}`);

        // Read response from queue
        const startTime = Date.now();

        while (true) {
            // Check timeout (5 seconds)
            if (Date.now() - startTime > 5000) {
                // log('Timeout waiting for info response');
                return null;
            }

            try {
                // Check if we have data in queue
                if (receiveQueue.size() >= 8) {
                    // Get 8 bytes from queue
                    const frame = receiveQueue.get(8);
                    if (frame) {
                        // log(`Retrieved 8 bytes from queue: ${Array.from(frame).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' ')}`);

                        // Check if it's an info response
                        if (frame[0] === 0x04) {
                            const length = frame[3];
                            // Extract data from the frame itself
                            const data = frame.slice(4, 4 + length);
                            // log(`Info response received, length: ${length}`);
                            return data;

                        } else {
                            // Unexpected response, log the frame
                            log(`Unexpected response frame: ${Array.from(frame).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' ')}`);
                            // Continue processing next frame
                            continue;
                        }
                    }
                } else {
                    // No data in queue, wait a bit before checking again
                    await new Promise(resolve => setTimeout(resolve, 1));
                    continue;
                }
            } catch (error) {
                log('Error reading response: ' + error.message);
                return null;
            }
        }
    }

    // Get bootloader version
    sendCommand(1)
        .then(data => {
            if (data && data.length >= 2) {
                bootloaderVersion = `${data[0]}.${data[1]}`;
                // log(`Bootloader version: ${bootloaderVersion}`);
            }
            // Get product series
            return sendCommand(2);
        })
        .then(data => {
            if (data && data.length >= 4) {
                productSeries = 0;
                for (let i = 3; i >= 0; i--) {
                    productSeries = (productSeries << 8) | data[i];
                }
                productSeries = `0x${productSeries.toString(16).toUpperCase().padStart(4, '0')}`;
                // log(`Product series: ${productSeries}`);
            }
            // Get product ID
            return sendCommand(3);
        })
        .then(data => {
            if (data && data.length >= 4) {
                productId = 0;
                for (let i = 3; i >= 0; i--) {
                    productId = (productId << 8) | data[i];
                }
                productId = `0x${productId.toString(16).toUpperCase().padStart(4, '0')}`;
                // log(`Product ID: ${productId}`);
            }

            // Update UI
            bootloaderInfoDiv.innerHTML = `
                <p>BOOTLOADER VERSION: ${bootloaderVersion}</p>
                <p>BOOTLODER PRODUCT SERIES: ${productSeries}</p>
                <p>BOOTLODER PRODUCT ID: ${productId}</p>
            `;
            // log('Bootloader info retrieval complete');
        })
        .catch(error => {
            log('Error getting bootloader info: ' + error.message);
            bootloaderInfoDiv.innerHTML = '<p>Error getting info</p>';
        });
}

// Handle file upload
function handleFileUpload(event) {
    const file = event.target.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = function (e) {
        firmwareData = new Uint8Array(e.target.result);
        firmwareSize = firmwareData.length;

        if (useCrcCheckbox.checked) {
            crc32Value = generate_crc32(firmwareData);
            log(`Firmware loaded: ${firmwareSize} bytes, CRC32: ${crc32Value}`);
        } else {
            log(`Firmware loaded: ${firmwareSize} bytes`);
        }

        updateProgress(0, 'Firmware loaded');
    };
    reader.readAsArrayBuffer(file);
}

// Real firmware flashing using Web Serial API
async function flashFirmware() {
    if (!isConnected || !writer || !reader) {
        log('Please connect to device first');
        return;
    }

    if (!firmwareData) {
        log('Please select a firmware file');
        return;
    }

    log('Starting firmware flash...');
    updateProgress(0, 'Sending header...');

    try {
        // Send header (firmware size)
        const frame1 = new Uint8Array(8);
        frame1[0] = 0x01; // Command type
        frame1[1] = 0x00; // Sequence low
        frame1[2] = 0x00; // Sequence high
        frame1[3] = 0x04; // Data length
        // Little endian firmware size
        frame1[4] = firmwareSize & 0xFF;
        frame1[5] = (firmwareSize >> 8) & 0xFF;
        frame1[6] = (firmwareSize >> 16) & 0xFF;
        frame1[7] = (firmwareSize >> 24) & 0xFF;

        log(`Sending firmware size: ${firmwareSize} bytes`);
        await writer.write(frame1);
        await waitForAck();
        log('Header sent');
        updateProgress(5, 'Header sent');

        // Send CRC32 if enabled
        if (useCrcCheckbox.checked && crc32Value !== null) {
            const frame2 = new Uint8Array(8);
            frame2[0] = 0x01; // Command type
            frame2[1] = 0x01; // Sequence low
            frame2[2] = 0x00; // Sequence high
            frame2[3] = 0x04; // Data length
            // Little endian CRC32
            frame2[4] = crc32Value & 0xFF;
            frame2[5] = (crc32Value >> 8) & 0xFF;
            frame2[6] = (crc32Value >> 16) & 0xFF;
            frame2[7] = (crc32Value >> 24) & 0xFF;

            log(`Sending CRC32: ${crc32Value}`);
            await writer.write(frame2);
            await waitForAck();
            log('CRC32 sent');
        }

        // Send data chunks
        const chunkSize = 4;
        const totalChunks = Math.ceil(firmwareSize / chunkSize);

        log(`Firmware size: ${firmwareSize} bytes, Total chunks: ${totalChunks}`);

        for (let i = 0; i < totalChunks; i++) {
            const start = i * chunkSize;
            const end = Math.min(start + chunkSize, firmwareSize);
            const chunk = firmwareData.slice(start, end);

            // Create frame (fixed 8 bytes)
            const frame = new Uint8Array(8);
            frame[0] = 0x02; // Command type
            frame[1] = i & 0xFF; // Sequence low
            frame[2] = (i >> 8) & 0xFF; // Sequence high
            frame[3] = chunkSize; // Always 4 bytes
            frame.set(chunk, 4); // Copy chunk data (bytes 4-7)
            // Remaining bytes are already 0 in new Uint8Array

            // Calculate progress
            const progress = Math.min(95, Math.round((i / totalChunks) * 90) + 5);
            updateProgress(progress, `Sending chunk ${i + 1}/${totalChunks} (seq=${i})`);

            // Add extra delay every 32 frames to avoid buffer overflow
            // if (i > 0 && i % 32 === 0) {
            //     log(`Extra delay after ${i} frames to avoid buffer overflow`);
            //     await new Promise(resolve => setTimeout(resolve, 100));
            // }

            // Send frame and wait for ACK with retry mechanism
            let retryCount = 0;
            const maxRetries = 3;
            let success = false;

            while (retryCount < maxRetries && !success) {
                try {
                    // log(`Sending chunk ${i + 1}/${totalChunks} (seq=${i})...`);
                    await writer.write(frame);
                    // Add delay to avoid overwhelming the device (increased to 50ms)
                    // await new Promise(resolve => setTimeout(resolve, 50));
                    // log(`Waiting for ACK for chunk ${i + 1} (attempt ${retryCount + 1}/${maxRetries})...`);
                    await waitForAck(10); // 1 second timeout for data chunks
                    // log(`Chunk ${i + 1}/${totalChunks} sent successfully`);
                    success = true;
                } catch (error) {
                    retryCount++;
                    // log(`Error sending chunk ${i + 1} (attempt ${retryCount}/${maxRetries}): ${error.message}`);
                    if (retryCount >= maxRetries) {
                        // log(`Failed to send chunk ${i + 1} after ${maxRetries} attempts`);
                        throw new Error(`Failed to send chunk ${i + 1}: ${error.message}`);
                    }
                    // Wait before retry (reduced to 500ms for faster recovery)
                    // await new Promise(resolve => setTimeout(resolve, 500));
                }
            }
        }

        // Send end command
        updateProgress(95, 'Sending end command...');
        const endFrame = new Uint8Array(8);
        endFrame[0] = 0x03; // Command type
        await writer.write(endFrame);
        log('End command sent');

        // Finalize
        updateProgress(100, 'Flashing complete');
        log('Firmware flashing completed successfully!');

    } catch (error) {
        log('Error during flashing: ' + error.message);
        updateProgress(0, 'Flashing failed');
    }
}

// Wait for ACK response with timeout
async function waitForAck(timeout = 5000) {
    const startTime = Date.now();
    let buffer = new Uint8Array(0);

    while (true) {
        // Check timeout
        if (Date.now() - startTime > timeout) {
            throw new Error('Timeout waiting for ACK/NAK response');
        }

        try {
            // Check if we have data in queue
            if (receiveQueue.size() >= 8) {
                // Get 8 bytes from queue
                const frame = receiveQueue.get(8);
                if (frame) {
                    // log(`Retrieved 8 bytes from queue: ${Array.from(frame).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' ')}`);

                    // Check if it's an ACK or NAK
                    if (frame[0] === 0x06 && frame[3] === 0x00 && frame[4] === 0x00 && frame[5] === 0x00 && frame[6] === 0x00 && frame[7] === 0x00) {
                        // ACK received
                        const seq = (frame[2] << 8) | frame[1];
                        // log(`ACK received for sequence: ${seq}`);
                        return true;
                    } else if (frame[0] === 0x15 && frame[3] === 0x00 && frame[4] === 0x00 && frame[5] === 0x00 && frame[6] === 0x00 && frame[7] === 0x00) {
                        // NAK received
                        const seq = (frame[2] << 8) | frame[1];
                        // log(`NAK received for sequence: ${seq}`);
                        throw new Error(`NAK received for sequence ${seq}`);
                    } else {
                        // Unexpected response, log the frame
                        // log(`Unexpected response frame: ${Array.from(frame).map(b => '0x' + b.toString(16).padStart(2, '0')).join(' ')}`);
                        // Continue processing next frame
                        continue;
                    }
                }
            } else {
                // No data in queue, wait a bit before checking again
                await new Promise(resolve => setTimeout(resolve, 1));
                continue;
            }
        } catch (error) {
            throw error;
        }
    }
}

// Event listeners
refreshPortsBtn.addEventListener('click', refreshSerialPorts);
connectBtn.addEventListener('click', connectToDevice);
disconnectBtn.addEventListener('click', disconnectFromDevice);
infoBtn.addEventListener('click', getBootloaderInfo);
flashBtn.addEventListener('click', flashFirmware);
firmwareFileInput.addEventListener('change', handleFileUpload);

// Initialize
function init() {
    refreshSerialPorts();
    disconnectBtn.disabled = true;
    infoBtn.disabled = true;
    flashBtn.disabled = true;
    log('STM32 Bootloader Flasher ready');
    updateProgress(0, 'Ready');
}

// Start initialization when DOM is fully loaded
document.addEventListener('DOMContentLoaded', init);
