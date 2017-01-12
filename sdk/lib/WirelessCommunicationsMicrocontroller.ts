import {Message} from './Message';
import {Microcontroller} from './Microcontroller';

const MAX_PAYLOAD_SIZE = 61;

export class WirelessCommunicationsMicrocontroller extends Microcontroller {
    reset() {
        const request = new Message(0x00, Buffer.alloc(1));
        const responseSize = Message.sizeWithPayload(1);
        return this.write(request.buffer)
            .then(() => this.read(responseSize))
            .then((buffer) => {
                if (!buffer.length)
                    throw new Error(`Response from ${this.name} was incorrect`);
                return buffer;
            });
    }

    send(buffer: Buffer) {
        let payload = buffer;
        if (payload.length > MAX_PAYLOAD_SIZE) {
            payload = payload.slice(0, MAX_PAYLOAD_SIZE);
        }
        if (payload.length < MAX_PAYLOAD_SIZE) {
            payload = Buffer.concat([payload, Buffer.alloc(MAX_PAYLOAD_SIZE - payload.length)]);
        }

        const request = new Message(0x04, payload);
        const responseSize = Message.sizeWithPayload(1);
        return this.write(request.buffer)
            .then(() => this.read(responseSize))
            .then((buffer) => {
                if (!buffer.length)
                    throw new Error(`Response from ${this.name} was incorrect`);
                return buffer;
            });
    }

    recv() {
        const request = new Message(0x03, Buffer.alloc(1));
        const responseSize = Message.sizeWithPayload(64);
        return this.write(request.buffer)
            .then(() => this.read(responseSize))
            .then((buffer) => {
                if (!buffer.length)
                    throw new Error(`Response from ${this.name} was incorrect`);
                return buffer;
            });
    }
}
