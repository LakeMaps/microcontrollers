const crc = require(`crc`);
const crc16 = (buffer: Buffer): Buffer => {
    const result = crc.crc16(buffer);
    return Buffer.from([result >> 8, result]);
};

export {
    crc16,
};
