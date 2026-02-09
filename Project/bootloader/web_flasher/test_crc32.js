// Test script for CRC32 calculation

// Copy of the fixed CRC32 function
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

// Test cases
const testCases = [
    { name: "Empty string", data: new Uint8Array([]), expected: 0x00000000 },
    { name: "Single byte 0x00", data: new Uint8Array([0x00]), expected: 0xD202EF8D },
    { name: "Single byte 0xFF", data: new Uint8Array([0xFF]), expected: 0xFFFFFFFF },
    { name: "ASCII '123456789'", data: new TextEncoder().encode("123456789"), expected: 0xCBF43926 },
    { name: "ASCII 'hello'", data: new TextEncoder().encode("hello"), expected: 0x3610A686 }
];

// Run tests
console.log("Testing CRC32 implementation...");
let allPassed = true;

testCases.forEach(test => {
    const result = generate_crc32(test.data);
    const passed = result === test.expected;
    allPassed &= passed;
    console.log(`${test.name}: 0x${result.toString(16).toUpperCase().padStart(8, '0')} ${passed ? '✓' : '✗ (expected: 0x' + test.expected.toString(16).toUpperCase().padStart(8, '0') + ')'}`);
});

console.log(`\nOverall: ${allPassed ? 'All tests passed!' : 'Some tests failed!'}`);