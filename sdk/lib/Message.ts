import {crc16} from './Checksum';

export class Message {
    static sizeWithPayload(payloadSize: number) {
        return payloadSize + 4;
    }

    public readonly buffer: Buffer;

    private readonly payloadSize: number;

    constructor(command: number, payload: Buffer) {
        const start = Buffer.from([0xAA, command]);
        const message = Buffer.concat([start, payload]);

        this.payloadSize = payload.length;
        this.buffer = Buffer.concat([message, crc16(message)]);
    }

    command() {
        return this.buffer[1];
    }

    payload() {
        return this.buffer.slice(2, this.payloadSize + 2);
    }

    checksum() {
        return this.buffer.slice(2 + this.payloadSize);
    }
}
