import {Message} from './Message';
import {Microcontroller} from './Microcontroller';

export class PropulsionMicrocontroller extends Microcontroller {
    reset() {
        const request = new Message(0x10, Buffer.alloc(1));
        const responseSize = Message.sizeWithPayload(1);
        return this.write(request.buffer)
            .then(() => this.read(responseSize))
            .then((buffer) => {
                if (!buffer.length)
                    throw new Error(`Response from ${this.name} was incorrect`);
                return buffer;
            });
    }

    setSpeed(m1: number, m2: number) {
        const payload = Buffer.alloc(8);
        payload.writeInt32BE(m1, 0);
        payload.writeInt32BE(m2, 4);

        const request = new Message(0x13, payload);
        const responseSize = Message.sizeWithPayload(8);
        return this.write(request.buffer)
            .then(() => this.read(responseSize))
            .then((buffer) => {
                if (!buffer.equals(request.buffer))
                    throw new Error(`Response from ${this.name} was incorrect`);
                return buffer;
            });
    }
}
