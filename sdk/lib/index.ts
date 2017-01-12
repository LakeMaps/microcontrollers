import { SerialPort } from './serial/SerialPort';
export { PropulsionMicrocontroller } from './PropulsionMicrocontroller';
export { WirelessCommunicationsMicrocontroller } from './WirelessCommunicationsMicrocontroller';

export function Factory<T>(klazz: new (name: string, read: (n: number) => Promise<Buffer>, write: (b: Buffer) => Promise<number>) => T, name: string): T {
    const s = new SerialPort(name);
    return new klazz(name, s.read(), s.write());
}
