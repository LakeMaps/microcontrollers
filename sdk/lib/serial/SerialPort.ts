import * as NativeSerialPort from 'serialport';

type ReadRequest = {
    n: number,
    callback: (data: Buffer) => void
};

export class SerialPort {
    private readonly port: NativeSerialPort;

    private readonly pendingReads : ReadRequest[] = [];

    private buffer: Buffer = Buffer.alloc(0);

    constructor(port: string) {
        this.port = new NativeSerialPort(port, {
            baudrate: 115200,
        });
        this.port.on(`data`, this.onData.bind(this));
    }

    onData(data?: Buffer) {
        if (data === undefined) {
            return;
        }

        this.buffer = Buffer.concat([this.buffer, data]);
        for (let i = 0; i < this.pendingReads.length; i++) {
            const nextRead = this.pendingReads[i];
            if (
                   (this.buffer[0] == 0xAA)
                && (this.buffer[1] == 0x0F || this.buffer[1] == 0x1F)
            ) {
                this.buffer = this.buffer.slice(5);
            }
            if (this.buffer.length >= nextRead.n) {
                const result = Buffer.alloc(nextRead.n);
                const leftovers = Buffer.alloc(this.buffer.length - nextRead.n);
                this.buffer.copy(result, 0, 0, nextRead.n);
                this.buffer.copy(leftovers, 0, nextRead.n);
                this.buffer = leftovers;
                this.pendingReads.shift();
                nextRead.callback(result);
            }
        }
    }

    read(): (n: number) => Promise<Buffer> {
        return (n: number) => new Promise((resolve) => {
            this.pendingReads.push({ n, callback: resolve });
        });
    }

    write(): (buffer: Buffer) => Promise<number> {
        return (buffer: Buffer) => new Promise<number>((resolve, reject) => {
            this.port.write(buffer, (err, count) => {
                if (err) {
                    return reject(err);
                }

                return resolve(count);
            });
        });
    }
}
