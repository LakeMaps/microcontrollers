import * as test from 'tape';
import {crc16} from '../lib/Checksum';

test(`CRC16 0xAA10`, (t) => {
    t.plan(1);

    const msg = Buffer.from([0xAA, 0x10]);
    const crc = Buffer.from([0x6C, 0x7F]);
    t.ok(crc16(msg).equals(crc));
});
