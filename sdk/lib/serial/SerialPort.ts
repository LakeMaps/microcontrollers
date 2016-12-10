import * as NativeSerialPort from 'serialport';

export class SerialPort {
    private readonly port: NativeSerialPort;

    private buffer: Buffer = Buffer.alloc(0);

    constructor(port: string) {
        this.port = new NativeSerialPort(port, {
            baudrate: 9600,
        });
    }

    onData(n: number, callback: (b: Buffer) => void, data?: Buffer) {
        if (data === undefined) {
            return;
        }

        this.buffer = Buffer.concat([this.buffer, data]);
        if (this.buffer.length < n) {
            return;
        }
        if (this.buffer.length == n) {
            const result = Buffer.alloc(n);
            this.buffer.copy(result, 0, 0, n);
            this.buffer = Buffer.alloc(0);
            callback(result);
        }
        if (this.buffer.length > n) {
            const result = Buffer.alloc(n);
            const leftovers = Buffer.alloc(this.buffer.length - n);
            this.buffer.copy(result, 0, 0, n);
            this.buffer.copy(leftovers, 0, n);
            this.buffer = leftovers;
            callback(result);
        }
    }

    read(): (n: number) => Promise<Buffer> {
        return (n: number) => new Promise((resolve) => this.port.on(`data`, this.onData.bind(this, n, resolve)));
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
